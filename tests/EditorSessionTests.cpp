#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Project/EditorSession.h"

namespace
{
    namespace fs = std::filesystem;

    static_assert(std::is_same_v<
        decltype(std::declval<const LimenEditor::EditorSession &>().GetConfiguration()),
        const Limen::ProjectConfiguration &
    >);
    static_assert(std::is_same_v<
        decltype(std::declval<const LimenEditor::EditorSession &>().GetAssetRegistry()),
        const Limen::AssetRegistry &
    >);
    // 项目会话必须同时拥有注册表，避免构造出 GetAssetRegistry 无法安全使用的会话。
    static_assert(!std::is_constructible_v<LimenEditor::EditorSession, Limen::ProjectConfiguration>);

    constexpr Limen::AssetID kAssetID{0x0011223344556677ULL, 0x8899aabbccddeeffULL};

    // 测试仅清理自己成功创建的临时目录，不碰仓库中的项目或资产。
    struct TemporaryProject
    {
        fs::path Root;

        TemporaryProject()
        {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            for (int attempt = 0; attempt < 100; ++attempt)
            {
                const auto candidate = fs::temp_directory_path() /
                    ("limen-editor-session-test-" + std::to_string(stamp) + "-" + std::to_string(attempt));
                std::error_code error;
                if (fs::create_directory(candidate, error))
                {
                    Root = candidate;
                    return;
                }
                if (error)
                    throw fs::filesystem_error("Cannot create test directory", candidate, error);
            }
            throw std::runtime_error("Cannot reserve a unique test directory");
        }

        ~TemporaryProject()
        {
            if (!Root.empty())
            {
                std::error_code ignored;
                fs::remove_all(Root, ignored);
            }
        }

        TemporaryProject(const TemporaryProject &) = delete;
        TemporaryProject &operator=(const TemporaryProject &) = delete;
    };

    struct CurrentPathGuard
    {
        fs::path Original = fs::current_path();

        explicit CurrentPathGuard(const fs::path &replacement)
        {
            fs::current_path(replacement);
        }

        ~CurrentPathGuard()
        {
            std::error_code ignored;
            fs::current_path(Original, ignored);
        }

        CurrentPathGuard(const CurrentPathGuard &) = delete;
        CurrentPathGuard &operator=(const CurrentPathGuard &) = delete;
    };

    LimenEditor::EditorSession MakeSession(Limen::ProjectConfiguration configuration)
    {
        // 注册表由会话独占；先用配置中的绝对资产目录加载，再移动配置。
        auto registry = Limen::CreateScope<Limen::AssetRegistry>(configuration.AssetRootAbsolute);
        Limen::AssetRegistryError error;
        if (!registry->TryLoad(error))
            throw std::runtime_error("Cannot load test asset registry: " +
                                     std::string(error.Message.c_str()));
        return LimenEditor::EditorSession(std::move(configuration), std::move(registry));
    }

    LimenEditor::EditorSession LoadSession(const fs::path &projectFile,
                                          const fs::path &unrelatedDirectory)
    {
        CurrentPathGuard cwd(unrelatedDirectory);
        Limen::ProjectConfiguration configuration;
        Limen::ProjectConfigurationError error;
        if (!Limen::TryLoadProjectConfiguration(projectFile, configuration, error))
            throw std::runtime_error("Cannot load test project: " + std::string(error.Message.c_str()));

        // configuration 在函数返回时销毁；会话必须独立拥有配置和已加载的注册表。
        return MakeSession(std::move(configuration));
    }

    bool Check(const char *name, const bool passed)
    {
        if (!passed)
            std::cerr << "FAIL: " << name << '\n';
        return passed;
    }
}

int main()
{
    try
    {
        TemporaryProject project;
        fs::create_directory(project.Root / "assets");
        fs::create_directory(project.Root / "unrelated-cwd");
        const auto projectFile = project.Root / "Game1.lmproject";
        {
            std::ofstream output(projectFile, std::ios::binary | std::ios::trunc);
            output << "schema_version = 1\n"
                   << "project_id = \"d0360f3d-ef2e-41d3-a0b6-9bcc5940a539\"\n"
                   << "display_name = \"Editor Session Test\"\n"
                   << "asset_root = \"assets\"\n";
            if (!output)
                throw std::runtime_error("Cannot write test project file");
        }
        {
            std::ofstream asset(project.Root / "assets" / "brick.bin", std::ios::binary | std::ios::trunc);
            asset << "asset bytes";
            if (!asset)
                throw std::runtime_error("Cannot write test asset");
        }
        {
            std::ofstream sidecar(project.Root / "assets" / "brick.bin.lmmeta",
                                  std::ios::binary | std::ios::trunc);
            sidecar << "schema_version = 1\n"
                    << "asset_id = \"00112233-4455-6677-8899-aabbccddeeff\"\n";
            if (!sidecar)
                throw std::runtime_error("Cannot write test asset sidecar");
        }

        const auto session = LoadSession(projectFile, project.Root / "unrelated-cwd");
        const auto &configuration = session.GetConfiguration();
        bool passed = true;
        passed &= Check("schema version retained", configuration.SchemaVersion == 1);
        passed &= Check("project ID retained",
                        configuration.ProjectID == "d0360f3d-ef2e-41d3-a0b6-9bcc5940a539");
        passed &= Check("display name retained", configuration.DisplayName == "Editor Session Test");
        passed &= Check("canonical project file retained",
                        configuration.ProjectFile == fs::canonical(projectFile));
        passed &= Check("relative asset root retained",
                        configuration.AssetRootRelative == fs::path("assets"));
        passed &= Check("canonical asset root retained",
                        configuration.AssetRootAbsolute == fs::canonical(project.Root / "assets"));

        Limen::AssetID foundID{};
        passed &= Check("session resolves sidecar asset path",
                        session.GetAssetRegistry().TryFindID("brick.bin", foundID));
        passed &= Check("session retains sidecar asset ID", foundID == kAssetID);
        Limen::AssetRecord foundRecord;
        passed &= Check("session resolves asset by ID",
                        session.GetAssetRegistry().TryFind(kAssetID, foundRecord));
        passed &= Check("session retains asset record",
                        foundRecord.ID == kAssetID &&
                        static_cast<std::string_view>(foundRecord.LogicalPath) == "brick.bin" &&
                        foundRecord.Status == Limen::AssetRecordStatus::Present);

        // 从另一个工作目录重开：资产身份只能来自项目 sidecar，不能依赖 CWD。
        const auto reopenedOriginal = LoadSession(projectFile, project.Root);
        Limen::AssetID reopenedID{};
        passed &= Check("reopened session resolves asset path",
                        reopenedOriginal.GetAssetRegistry().TryFindID("brick.bin", reopenedID));
        passed &= Check("asset ID survives session reopen from another cwd", reopenedID == kAssetID);

        // 新建项目与重新打开项目应形成同一份会话身份，且不依赖进程工作目录。
        const auto createdDirectory = project.Root / "Created";
        const auto createdProjectFile = createdDirectory / "Created.lmproject";
        Limen::ProjectConfiguration createdConfiguration;
        Limen::ProjectConfigurationError createError;
        bool didCreate = false;
        {
            CurrentPathGuard cwd(project.Root / "unrelated-cwd");
            didCreate = Limen::TryCreateProjectConfiguration(
                createdDirectory, "Created in Editor", createdConfiguration, createError);
        }
        if (!didCreate)
            std::cerr << "Create failure: " << createError.Message.c_str() << '\n';
        passed &= Check("project can be created before Editor session opens", didCreate);
        if (didCreate)
        {
            const auto createdSession = MakeSession(std::move(createdConfiguration));
            const auto reopenedSession = LoadSession(createdProjectFile, project.Root);
            const auto &created = createdSession.GetConfiguration();
            const auto &reopened = reopenedSession.GetConfiguration();
            passed &= Check("reopened Editor session keeps project identity",
                            reopened.ProjectID == created.ProjectID &&
                            reopened.DisplayName == created.DisplayName &&
                            reopened.ProjectFile == created.ProjectFile);
            passed &= Check("reopened Editor session keeps asset root",
                            reopened.AssetRootAbsolute == created.AssetRootAbsolute &&
                            reopened.AssetRootAbsolute == fs::canonical(createdDirectory / "assets"));
        }
        return passed ? 0 : 1;
    }
    catch (const std::exception &exception)
    {
        std::cerr << "FAIL: " << exception.what() << '\n';
        return 1;
    }
}
