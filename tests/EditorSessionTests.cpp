#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <glm/gtc/matrix_transform.hpp>

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
    static_assert(std::is_same_v<
        decltype(std::declval<const LimenEditor::EditorSession &>().GetScene()),
        const Limen::Scene &
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

    glm::mat4 Translation(const float x, const float y, const float z)
    {
        return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    }

    bool MatchesTransform(const glm::mat4 &actual, const glm::mat4 &expected)
    {
        for (int column = 0; column < 4; ++column)
        {
            for (int row = 0; row < 4; ++row)
            {
                if (actual[column][row] != expected[column][row])
                    return false;
            }
        }
        return true;
    }

    // 仅作为 CPU 侧资源身份标记，测试不会解引用。
    Limen::Ref<Limen::Mesh> MakeMeshToken()
    {
        return Limen::Ref<Limen::Mesh>(
            reinterpret_cast<Limen::Mesh *>(std::uintptr_t{1}),
            [](Limen::Mesh *) {});
    }

    Limen::Ref<Limen::Material> MakeMaterialToken()
    {
        return Limen::Ref<Limen::Material>(
            reinterpret_cast<Limen::Material *>(std::uintptr_t{2}),
            [](Limen::Material *) {});
    }

    bool TestActiveSceneContract(const Limen::ProjectConfiguration &configuration)
    {
        // 会话独占活动场景；CreateObject 只把稳定 ID 暴露给调用方。
        LimenEditor::EditorSession session = MakeSession(configuration);

        const Limen::SceneObjectID first = session.CreateObject();
        bool passed = Check("session creates a valid scene object ID", first.isValid());

        Limen::SceneObjectHandle found{};
        passed &= Check("session scene contains the created ID",
                        session.GetScene().TryFindObject(first, found));
        passed &= Check("created object is alive in the session scene",
                        found.IsValid() && session.GetScene().IsObjectAlive(found));

        const Limen::SceneObjectID second = session.CreateObject();
        passed &= Check("second created object has a distinct valid ID",
                        second.isValid() && !(second == first));

        return passed;
    }

    bool TestSelectionContract(const Limen::ProjectConfiguration &configuration)
    {
        LimenEditor::EditorSession session = MakeSession(configuration);

        const Limen::SceneObjectID first = session.CreateObject();
        bool passed = Check("created object becomes selected", first.isValid());
        passed &= Check("session reports created object as selected",
                        session.GetSelectedObjectID() == first);
        passed &= Check("IsObjectSelected agrees for created object",
                        session.IsObjectSelected(first));

        const Limen::SceneObjectID second = session.CreateObject();
        passed &= Check("newest object takes over selection",
                        session.GetSelectedObjectID() == second);
        passed &= Check("previous object is no longer selected",
                        !session.IsObjectSelected(first));
        passed &= Check("IsObjectSelected agrees for newest object",
                        session.IsObjectSelected(second));

        passed &= Check("selecting a live object succeeds",
                        session.SelectObject(first));
        passed &= Check("selection switches back",
                        session.GetSelectedObjectID() == first &&
                        !session.IsObjectSelected(second));

        // 选择失效 ID 必须失败，并且不能改变当前选择。
        const Limen::SceneObjectID stale{
            0xDEADBEEFDEADBEEFULL,
            0xDEADBEEFDEADBEEFULL
        };
        passed &= Check("selecting a stale ID is rejected",
                        !session.SelectObject(stale));
        passed &= Check("failed select keeps previous selection",
                        session.GetSelectedObjectID() == first);

        session.ClearSelection();
        passed &= Check("clear selection yields invalid ID",
                        !session.GetSelectedObjectID().isValid());
        passed &= Check("no object is selected after clear",
                        !session.IsObjectSelected(first) &&
                        !session.IsObjectSelected(second));

        passed &= Check("selection works again after clear",
                        session.SelectObject(second) &&
                        session.GetSelectedObjectID() == second);

        return passed;
    }

    bool TestDirtyAndDeleteContract(const Limen::ProjectConfiguration &configuration)
    {
        LimenEditor::EditorSession session = MakeSession(configuration);

        bool passed = Check("fresh session is not dirty", !session.IsDirty());

        const Limen::SceneObjectID first = session.CreateObject();
        passed &= Check("creating an object marks the session dirty",
                        first.isValid() && session.IsDirty());

        session.ClearDirty();
        passed &= Check("ClearDirty resets dirty state", !session.IsDirty());

        // 无选择时编辑必须失败，且不能改脏位、不能改 out 参数。
        session.ClearSelection();
        const glm::mat4 sentinel = Translation(90.0f, 91.0f, 92.0f);
        glm::mat4 out = sentinel;
        passed &= Check("transform query fails without selection",
                        !session.TryGetSelectedObjectTransform(out));
        passed &= Check("failed transform query preserves out",
                        MatchesTransform(out, sentinel));
        passed &= Check("failed edit keeps session clean",
                        !session.SetSelectedObjectTransform(
                            Translation(1.0f, 2.0f, 3.0f)) &&
                        !session.IsDirty());

        // 选中后编辑：成功、可读回、置脏。
        passed &= Check("reselect first object", session.SelectObject(first));
        const glm::mat4 moved = Translation(4.0f, 5.0f, 6.0f);
        passed &= Check("selected object accepts transform",
                        session.SetSelectedObjectTransform(moved));
        passed &= Check("editing marks the session dirty", session.IsDirty());
        out = glm::mat4(0.0f);
        passed &= Check("edited transform can be read back",
                        session.TryGetSelectedObjectTransform(out));
        passed &= Check("read-back transform matches edit",
                        MatchesTransform(out, moved));

        // 删除当前选中对象：成功、置脏、清选择、旧 ID 失效。
        const Limen::SceneObjectID second = session.CreateObject();
        session.ClearDirty();
        passed &= Check("delete selected object succeeds",
                        session.DeleteObject(second));
        passed &= Check("deleting marks the session dirty", session.IsDirty());
        passed &= Check("deleted object is no longer selected",
                        !session.GetSelectedObjectID().isValid() &&
                        !session.IsObjectSelected(second));
        passed &= Check("deleted ID can no longer be selected",
                        !session.SelectObject(second));

        // 删除未选中对象：选择保持、仍置脏。
        session.ClearDirty();
        passed &= Check("select survivor", session.SelectObject(first));
        const Limen::SceneObjectID victim = session.CreateObject();
        passed &= Check("switch selection back to survivor",
                        session.SelectObject(first));
        session.ClearDirty();
        passed &= Check("delete non-selected object succeeds",
                        session.DeleteObject(victim));
        passed &= Check("deleting non-selected object keeps selection",
                        session.GetSelectedObjectID() == first);
        passed &= Check("delete marks dirty again", session.IsDirty());

        // 删除失效 ID：失败且不改脏位。
        session.ClearDirty();
        passed &= Check("deleting a stale ID fails", !session.DeleteObject(victim));
        passed &= Check("failed delete keeps session clean", !session.IsDirty());

        return passed;
    }

    bool TestRenderableObjectContract(
        const Limen::ProjectConfiguration &configuration)
    {
        LimenEditor::EditorSession session = MakeSession(configuration);
        const auto mesh = MakeMeshToken();
        const auto material = MakeMaterialToken();

        const Limen::SceneObjectID id =
            session.CreateRenderableObject(mesh, material);
        bool passed = Check("renderable object ID is valid", id.isValid());
        passed &= Check("renderable object is selected",
                        session.GetSelectedObjectID() == id);
        passed &= Check("creating renderable object marks dirty",
                        session.IsDirty());

        int visits = 0;
        session.GetScene().ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &,
                const Limen::SceneObjectID &visitedID,
                const Limen::Ref<Limen::Mesh> &meshResource,
                const Limen::Ref<Limen::Material> &materialResource,
                const glm::mat4 &transform)
            {
                ++visits;
                passed &= Check("renderable traversal ID matches",
                                visitedID == id);
                passed &= Check("renderable traversal mesh matches",
                                meshResource == mesh);
                passed &= Check("renderable traversal material matches",
                                materialResource == material);
                passed &= Check("renderable object starts at identity",
                                MatchesTransform(transform, glm::mat4{1.0f}));
            });
        passed &= Check("one renderable object is visited", visits == 1);

        passed &= Check("empty renderable mesh is rejected",
                        !session.CreateRenderableObject({}, material).isValid());
        passed &= Check("empty renderable material is rejected",
                        !session.CreateRenderableObject(mesh, {}).isValid());

        session.ClearDirty();
        passed &= Check("delete renderable object succeeds",
                        session.DeleteObject(id));
        passed &= Check("deleting renderable clears selection",
                        !session.GetSelectedObjectID().isValid());
        passed &= Check("deleting renderable marks dirty", session.IsDirty());

        visits = 0;
        session.GetScene().ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &,
                const Limen::SceneObjectID &,
                const Limen::Ref<Limen::Mesh> &,
                const Limen::Ref<Limen::Material> &,
                const glm::mat4 &)
            {
                ++visits;
            });
        passed &= Check("deleted renderable is not visited", visits == 0);
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
        passed &= TestActiveSceneContract(configuration);
        passed &= TestSelectionContract(configuration);
        passed &= TestDirtyAndDeleteContract(configuration);
        passed &= TestRenderableObjectContract(configuration);
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
