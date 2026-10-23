#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Limen/Project/ProjectConfiguration.h"

namespace
{
    namespace fs = std::filesystem;

    constexpr std::string_view kProjectID = "d0360f3d-ef2e-41d3-a0b6-9bcc5940a539";

    struct TestRunner
    {
        int Checks = 0;
        int Failures = 0;

        void Check(const std::string_view name, const bool passed)
        {
            ++Checks;
            if (!passed)
            {
                ++Failures;
                std::cerr << "FAIL: " << name << '\n';
            }
        }
    };

    // 所有用例只在自身创建的临时目录内写入和清理，不依赖仓库工作目录。
    struct TemporaryProject
    {
        fs::path Root;

        TemporaryProject()
        {
            const auto base = fs::temp_directory_path();
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            for (int attempt = 0; attempt < 100; ++attempt)
            {
                const auto candidate = base / ("limen-project-config-test-" +
                                               std::to_string(stamp) + "-" + std::to_string(attempt));
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

    struct ScopedCurrentPath
    {
        fs::path Previous = fs::current_path();

        explicit ScopedCurrentPath(const fs::path &next)
        {
            fs::current_path(next);
        }

        ~ScopedCurrentPath()
        {
            std::error_code ignored;
            fs::current_path(Previous, ignored);
        }

        ScopedCurrentPath(const ScopedCurrentPath &) = delete;
        ScopedCurrentPath &operator=(const ScopedCurrentPath &) = delete;
    };

    void WriteFile(const fs::path &path, const std::string_view text)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream)
            throw std::runtime_error("Cannot open test project file");
        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!stream)
            throw std::runtime_error("Cannot write test project file");
    }

    std::string ValidConfig(const std::string_view assetRoot = "assets",
                            const std::string_view displayName = {})
    {
        std::string result = "schema_version = 1\nproject_id = \"";
        result += kProjectID;
        result += "\"\n";
        if (!displayName.empty())
        {
            result += "display_name = \"";
            result += displayName;
            result += "\"\n";
        }
        result += "asset_root = \"";
        result += assetRoot;
        result += "\"\n";
        return result;
    }

    Limen::ProjectConfiguration SentinelConfiguration()
    {
        return {
            .SchemaVersion = 47,
            .ProjectID = "previous-id",
            .DisplayName = "previous-name",
            .ProjectFile = "previous-project.lmproject",
            .AssetRootRelative = "previous-assets",
            .AssetRootAbsolute = "/previous-assets"
        };
    }

    bool SameConfiguration(const Limen::ProjectConfiguration &a,
                           const Limen::ProjectConfiguration &b)
    {
        return a.SchemaVersion == b.SchemaVersion && a.ProjectID == b.ProjectID &&
               a.DisplayName == b.DisplayName && a.ProjectFile == b.ProjectFile &&
               a.AssetRootRelative == b.AssetRootRelative &&
               a.AssetRootAbsolute == b.AssetRootAbsolute;
    }

    void ExpectFailure(TestRunner &runner, const std::string_view name,
                       const fs::path &projectFile, const std::string_view content,
                       const Limen::ProjectConfigurationErrorCode expectedCode,
                       const std::string_view expectedKey)
    {
        WriteFile(projectFile, content);
        const auto original = SentinelConfiguration();
        auto output = original;
        Limen::ProjectConfigurationError error;
        const bool loaded = Limen::TryLoadProjectConfiguration(projectFile, output, error);
        const std::string label(name);
        runner.Check(label + ": rejected", !loaded);
        if (loaded)
            return;
        runner.Check(label + ": error code", error.Code == expectedCode);
        runner.Check(label + ": error file", error.File == projectFile);
        runner.Check(label + ": error key",
                     error.Key == Limen::LString(std::string(expectedKey)));
        runner.Check(label + ": output unchanged", SameConfiguration(output, original));
    }

    void TestValidProjects(TestRunner &runner, const fs::path &projectRoot,
                           const fs::path &otherDirectory)
    {
        const auto projectFile = projectRoot / "Game1.lmproject";
        WriteFile(projectFile, ValidConfig());
        const auto expectedFile = fs::canonical(projectFile);
        const auto expectedAssets = fs::canonical(projectRoot / "assets");
        Limen::ProjectConfiguration output;
        Limen::ProjectConfigurationError error = {
            .Code = Limen::ProjectConfigurationErrorCode::InvalidField,
            .File = "old.lmproject",
            .Key = "old-key",
            .Message = "old-message"
        };
        runner.Check("minimal project loads",
                     Limen::TryLoadProjectConfiguration(projectFile, output, error));
        runner.Check("minimal project fields",
                     output.SchemaVersion == 1 && output.ProjectID == Limen::LString(std::string(kProjectID)) &&
                     output.DisplayName == "Game1" && output.ProjectFile == expectedFile &&
                     output.AssetRootRelative == fs::path("assets") &&
                     output.AssetRootAbsolute == expectedAssets);
        runner.Check("success clears previous error",
                     error.Code == Limen::ProjectConfigurationErrorCode::None &&
                     error.File.empty() && error.Key.empty() && error.Message.empty());

        WriteFile(projectFile, ValidConfig("assets", "Explicit Name"));
        runner.Check("explicit display name loads",
                     Limen::TryLoadProjectConfiguration(projectFile, output, error));
        runner.Check("explicit display name is kept", output.DisplayName == "Explicit Name");

        // 配置中的相对路径必须以 .lmproject 所在目录为基准，而不是进程的当前目录。
        const auto previousDirectory = fs::current_path();
        fs::current_path(otherDirectory);
        const bool loadedOutsideProject = Limen::TryLoadProjectConfiguration(projectFile, output, error);
        fs::current_path(previousDirectory);
        runner.Check("load is independent of current directory", loadedOutsideProject &&
                     output.AssetRootAbsolute == expectedAssets);

        const auto nestedAssets = projectRoot / "assets" / "Textures";
        fs::create_directory(nestedAssets);
        WriteFile(projectFile, ValidConfig("assets/Textures"));
        runner.Check("nested asset root loads",
                     Limen::TryLoadProjectConfiguration(projectFile, output, error));
        runner.Check("nested asset root keeps exact relative path",
                     output.AssetRootRelative == fs::path("assets/Textures") &&
                     output.AssetRootAbsolute == fs::canonical(nestedAssets));

        // 项目内的链接可以使用；拼写校验检查链接名及其后续分量，最终位置仍由 canonical 校验。
        std::error_code internalLinkError;
        fs::create_directory_symlink(projectRoot / "assets", projectRoot / "assets-link", internalLinkError);
        if (!internalLinkError)
        {
            WriteFile(projectFile, ValidConfig("assets-link/Textures"));
            runner.Check("internal asset symlink loads",
                         Limen::TryLoadProjectConfiguration(projectFile, output, error));
            runner.Check("internal asset symlink resolves inside project",
                         output.AssetRootAbsolute == fs::canonical(nestedAssets));
        }
        else
            std::cout << "SKIP: internal symlink fixture (" << internalLinkError.message() << ")\n";

        const std::u8string utf8Stem = u8"示例项目";
        const std::u8string utf8Root = u8"资源";
        const auto unicodeFile = projectRoot / fs::path(std::u8string(u8"示例项目.lmproject"));
        const auto unicodeAssets = projectRoot / fs::path(utf8Root);
        fs::create_directory(unicodeAssets);
        WriteFile(unicodeFile, ValidConfig(std::string(utf8Root.begin(), utf8Root.end())));
        runner.Check("UTF-8 project and asset path load",
                     Limen::TryLoadProjectConfiguration(unicodeFile, output, error));
        runner.Check("UTF-8 filename fallback and asset path",
                     output.DisplayName == Limen::LString(std::string(utf8Stem.begin(), utf8Stem.end())) &&
                     output.AssetRootAbsolute == fs::canonical(unicodeAssets));
    }

    void TestInvalidProjects(TestRunner &runner, const fs::path &projectRoot,
                             const fs::path &outsideDirectory)
    {
        using Code = Limen::ProjectConfigurationErrorCode;
        const auto file = projectRoot / "Invalid.lmproject";
        // 项目文件后缀是跨 Editor/Player 共享的格式契约，不应只在某个客户端检查。
        ExpectFailure(runner, "legacy project suffix", projectRoot / "Invalid.lproj",
                      ValidConfig(), Code::InvalidPath, "");
        ExpectFailure(runner, "malformed TOML", file, "schema_version = ", Code::InvalidToml, "");
        ExpectFailure(runner, "duplicate field", file,
                      ValidConfig() + "asset_root = \"assets\"\n", Code::InvalidToml, "");
        ExpectFailure(runner, "missing schema version", file,
                      "project_id = \"d0360f3d-ef2e-41d3-a0b6-9bcc5940a539\"\nasset_root = \"assets\"\n",
                      Code::InvalidField, "schema_version");
        ExpectFailure(runner, "wrong schema type", file,
                      "schema_version = \"1\"\nproject_id = \"d0360f3d-ef2e-41d3-a0b6-9bcc5940a539\"\nasset_root = \"assets\"\n",
                      Code::InvalidField, "schema_version");
        ExpectFailure(runner, "unsupported schema", file,
                      "schema_version = 2\nproject_id = \"d0360f3d-ef2e-41d3-a0b6-9bcc5940a539\"\nasset_root = \"assets\"\n",
                      Code::UnsupportedSchema, "schema_version");
        ExpectFailure(runner, "missing project ID", file,
                      "schema_version = 1\nasset_root = \"assets\"\n",
                      Code::InvalidField, "project_id");
        ExpectFailure(runner, "invalid UUID", file,
                      "schema_version = 1\nproject_id = \"not-a-uuid\"\nasset_root = \"assets\"\n",
                      Code::InvalidField, "project_id");
        ExpectFailure(runner, "zero UUID", file,
                      "schema_version = 1\nproject_id = \"00000000-0000-0000-0000-000000000000\"\nasset_root = \"assets\"\n",
                      Code::InvalidField, "project_id");
        ExpectFailure(runner, "empty display name", file,
                      ValidConfig() + "display_name = \"\"\n", Code::InvalidField, "display_name");
        ExpectFailure(runner, "unknown root key", file,
                      ValidConfig() + "typo = 1\n", Code::InvalidField, "typo");
        ExpectFailure(runner, "missing asset root", file,
                      "schema_version = 1\nproject_id = \"d0360f3d-ef2e-41d3-a0b6-9bcc5940a539\"\n",
                      Code::InvalidField, "asset_root");
        ExpectFailure(runner, "empty asset root", file, ValidConfig(""),
                      Code::InvalidField, "asset_root");

        struct InvalidRoot
        {
            std::string_view Name;
            std::string_view Value;
        };
        constexpr InvalidRoot invalidRoots[] = {
            {"parent traversal", "assets/../outside"},
            {"current-directory component", "./assets"},
            {"absolute root", "/tmp"},
            {"drive prefix", "C:/outside"},
            {"double separator", "assets//nested"},
            {"trailing separator", "assets/"},
            {"trailing dot", "assets."},
            {"trailing space", "assets "},
            {"portable-name restriction", "assets|x"},
            {"missing directory", "not-created"}
        };
        for (const auto &test : invalidRoots)
            ExpectFailure(runner, test.Name, file, ValidConfig(test.Value),
                          Code::InvalidPath, "asset_root");

        // macOS 允许某些 Windows 保留设备名；项目文件不能因此变成不可移植。
        std::error_code reservedNameError;
        const auto reservedName = projectRoot / "CON";
        if (fs::create_directory(reservedName, reservedNameError))
            ExpectFailure(runner, "Windows reserved directory name", file,
                          ValidConfig("CON"), Code::InvalidPath, "asset_root");
        else
            std::cout << "SKIP: reserved-name fixture (" << reservedNameError.message() << ")\n";

        // 仅在文件系统大小写不敏感时，才能构造“拼写不符但仍可访问”的用例。
        std::error_code caseError;
        if (fs::equivalent(projectRoot / "ASSETS", projectRoot / "assets", caseError))
            ExpectFailure(runner, "asset root case mismatch", file,
                          ValidConfig("ASSETS"), Code::InvalidPath, "asset_root");
        else
            std::cout << "SKIP: case-mismatch fixture (filesystem is case-sensitive)\n";

        std::error_code nestedCaseError;
        if (fs::equivalent(projectRoot / "assets" / "TEXTURES",
                           projectRoot / "assets" / "Textures", nestedCaseError))
            ExpectFailure(runner, "nested asset root case mismatch", file,
                          ValidConfig("assets/TEXTURES"), Code::InvalidPath, "asset_root");
        else
            std::cout << "SKIP: nested case-mismatch fixture (filesystem is case-sensitive)\n";

        ExpectFailure(runner, "backslash", file,
                      "schema_version = 1\nproject_id = \"d0360f3d-ef2e-41d3-a0b6-9bcc5940a539\"\n"
                      "asset_root = 'assets\\nested'\n", Code::InvalidPath, "asset_root");
        ExpectFailure(runner, "control character", file,
                      "schema_version = 1\nproject_id = \"d0360f3d-ef2e-41d3-a0b6-9bcc5940a539\"\n"
                      "asset_root = \"\\tassets\"\n", Code::InvalidPath, "asset_root");

        WriteFile(projectRoot / "not-a-directory", "content");
        ExpectFailure(runner, "asset root is a file", file, ValidConfig("not-a-directory"),
                      Code::InvalidPath, "asset_root");

        // canonical 必须解析符号链接，并按目录分量判断是否逃逸；相似字符串前缀也不算项目内。
        std::error_code symlinkError;
        fs::create_directory_symlink(outsideDirectory, projectRoot / "outside-link", symlinkError);
        if (!symlinkError)
            ExpectFailure(runner, "symlink escapes project", file, ValidConfig("outside-link"),
                          Code::InvalidPath, "asset_root");
        else
            std::cout << "SKIP: symlink test (" << symlinkError.message() << ")\n";

        const auto missingFile = projectRoot / "Missing.lmproject";
        const auto original = SentinelConfiguration();
        auto output = original;
        Limen::ProjectConfigurationError error;
        runner.Check("missing project file rejected",
                     !Limen::TryLoadProjectConfiguration(missingFile, output, error) &&
                     error.Code == Code::FileAccess && error.File == missingFile &&
                     SameConfiguration(output, original));
        const auto projectDirectory = projectRoot / "Directory.lmproject";
        fs::create_directory(projectDirectory);
        runner.Check("directory is not a project file",
                     !Limen::TryLoadProjectConfiguration(projectDirectory, output, error) &&
                     error.Code == Code::FileAccess && error.File == projectDirectory &&
                     SameConfiguration(output, original));
    }

    void TestProjectCreation(TestRunner &runner, const fs::path &temporaryRoot,
                             const fs::path &otherDirectory)
    {
        using Code = Limen::ProjectConfigurationErrorCode;
        const auto createdDirectory = temporaryRoot / "Created";
        const auto createdFile = createdDirectory / "Created.lmproject";
        const Limen::LString displayName("Demo \"Quoted\"\nSecond line");

        Limen::ProjectConfiguration created = SentinelConfiguration();
        Limen::ProjectConfigurationError error = {
            .Code = Code::InvalidField,
            .File = "previous.lmproject",
            .Key = "previous-key",
            .Message = "previous-message"
        };
        bool firstCreated = false;

        // 创建和重新打开都从无关工作目录执行；绝对项目路径不应依赖进程 CWD。
        {
            ScopedCurrentPath outsideProject(otherDirectory);
            firstCreated = Limen::TryCreateProjectConfiguration(
                createdDirectory, displayName, created, error);
            if (!firstCreated)
                std::cerr << "Create failure: code=" << static_cast<int>(error.Code)
                          << ", file=" << error.File
                          << ", key=" << std::string_view(error.Key.data(), error.Key.size())
                          << ", message=" << std::string_view(error.Message.data(), error.Message.size())
                          << '\n';
            runner.Check("new project is created", firstCreated);
            if (firstCreated)
            {
                const bool layoutExists = fs::is_regular_file(createdFile) &&
                                          fs::is_directory(createdDirectory / "assets");
                runner.Check("created project layout", layoutExists);
                if (layoutExists)
                    runner.Check("created project fields",
                                 created.SchemaVersion == 1 &&
                                 created.ProjectFile == fs::canonical(createdFile) &&
                                 created.AssetRootRelative == fs::path("assets") &&
                                 created.AssetRootAbsolute == fs::canonical(createdDirectory / "assets") &&
                                 created.DisplayName == displayName);
                runner.Check("creation clears previous error",
                             error.Code == Code::None && error.File.empty() &&
                             error.Key.empty() && error.Message.empty());

                const std::string_view id(created.ProjectID.data(), created.ProjectID.size());
                runner.Check("created project ID is UUID v4",
                             id.size() == 36 && id[8] == '-' && id[13] == '-' &&
                             id[18] == '-' && id[23] == '-' && id[14] == '4' &&
                             (id[19] == '8' || id[19] == '9' ||
                              id[19] == 'a' || id[19] == 'b'));
                auto temporaryFile = createdFile;
                temporaryFile += ".tmp";
                runner.Check("successful creation leaves no temporary project file",
                             !fs::exists(temporaryFile));

                Limen::ProjectConfiguration reopened;
                Limen::ProjectConfigurationError loadError;
                const bool didReopen = layoutExists &&
                    Limen::TryLoadProjectConfiguration(createdFile, reopened, loadError);
                runner.Check("created project reopens from another CWD", didReopen);
                if (didReopen)
                    runner.Check("created project survives round trip",
                                 SameConfiguration(reopened, created) &&
                                 reopened.ProjectID != Limen::LString(
                                     "00000000-0000-0000-0000-000000000000"));
            }
        }

        const auto secondDirectory = temporaryRoot / "Second";
        Limen::ProjectConfiguration second;
        const bool secondCreated = Limen::TryCreateProjectConfiguration(
            secondDirectory, "Second", second, error);
        if (!secondCreated)
            std::cerr << "Second create failure: code=" << static_cast<int>(error.Code)
                      << ", file=" << error.File
                      << ", key=" << std::string_view(error.Key.data(), error.Key.size())
                      << ", message=" << std::string_view(error.Message.data(), error.Message.size())
                      << '\n';
        runner.Check("second project is created", secondCreated);
        if (firstCreated && secondCreated)
            runner.Check("new projects receive different IDs",
                         second.ProjectID != created.ProjectID);

        // 已存在目录属于调用方：即使里面没有项目文件，创建失败也不能清理或覆盖它。
        const auto existingDirectory = temporaryRoot / "Existing";
        fs::create_directory(existingDirectory);
        const auto marker = existingDirectory / "keep.txt";
        WriteFile(marker, "keep this file");
        const auto original = SentinelConfiguration();
        auto rejectedOutput = original;
        const bool acceptedExisting = Limen::TryCreateProjectConfiguration(
            existingDirectory, "Existing", rejectedOutput, error);
        runner.Check("existing project directory is rejected", !acceptedExisting);
        runner.Check("existing directory failure preserves output",
                     SameConfiguration(rejectedOutput, original));
        runner.Check("existing directory and file are untouched",
                     fs::is_regular_file(marker) && fs::file_size(marker) == 14 &&
                     !fs::exists(existingDirectory / "Existing.lmproject") &&
                     !fs::exists(existingDirectory / "assets"));
        runner.Check("existing directory failure reports an error",
                     error.Code != Code::None && !error.File.empty() && !error.Message.empty());

        const auto emptyNameDirectory = temporaryRoot / "EmptyName";
        rejectedOutput = original;
        const bool acceptedEmptyName = Limen::TryCreateProjectConfiguration(
            emptyNameDirectory, "", rejectedOutput, error);
        runner.Check("empty display name is rejected", !acceptedEmptyName);
        runner.Check("invalid display name leaves no partial project",
                     !fs::exists(emptyNameDirectory) && SameConfiguration(rejectedOutput, original));
        runner.Check("invalid display name reports its key",
                     error.Code == Code::InvalidField && error.Key == "display_name");

        // macOS 可创建以点结尾的目录，但共享项目名不能在 Windows 上失效。
        const auto nonPortableDirectory = temporaryRoot / "TrailingDot.";
        rejectedOutput = original;
        const bool acceptedNonPortableName = Limen::TryCreateProjectConfiguration(
            nonPortableDirectory, "Trailing Dot", rejectedOutput, error);
        runner.Check("non-portable project directory is rejected", !acceptedNonPortableName);
        runner.Check("invalid project path leaves no partial project",
                     !fs::exists(nonPortableDirectory) && SameConfiguration(rejectedOutput, original));
        runner.Check("trailing dot reports invalid path", error.Code == Code::InvalidPath);

        // 相对路径不应随着当前工作目录变化而意外创建项目。
        {
            ScopedCurrentPath outsideProject(otherDirectory);
            const fs::path relativeDirectory("RelativeProject");
            rejectedOutput = original;
            const bool acceptedRelativePath = Limen::TryCreateProjectConfiguration(
                relativeDirectory, "Relative", rejectedOutput, error);
            runner.Check("relative project path is rejected",
                         !acceptedRelativePath && error.Code == Code::InvalidPath &&
                         SameConfiguration(rejectedOutput, original));
            runner.Check("relative project path creates no directory",
                         !fs::exists(otherDirectory / relativeDirectory));
        }

        const auto missingParentDirectory = temporaryRoot / "MissingParent" / "Child";
        rejectedOutput = original;
        const bool acceptedMissingParent = Limen::TryCreateProjectConfiguration(
            missingParentDirectory, "Child", rejectedOutput, error);
        runner.Check("missing parent is rejected",
                     !acceptedMissingParent && error.Code == Code::InvalidPath &&
                     SameConfiguration(rejectedOutput, original));
        runner.Check("missing parent failure creates no directory",
                     !fs::exists(missingParentDirectory.parent_path()));

        // Windows 保留设备名即使带扩展名也不可作为跨平台项目目录名。
        const auto reservedDirectory = temporaryRoot / "CON.txt";
        rejectedOutput = original;
        const bool acceptedReservedName = Limen::TryCreateProjectConfiguration(
            reservedDirectory, "Reserved", rejectedOutput, error);
        runner.Check("Windows reserved project name is rejected",
                     !acceptedReservedName && error.Code == Code::InvalidPath &&
                     SameConfiguration(rejectedOutput, original));
        runner.Check("reserved name failure creates no directory",
                     !fs::exists(reservedDirectory));
    }
}

int main()
{
    try
    {
        TestRunner runner;
        TemporaryProject temporary;
        const auto projectRoot = temporary.Root / "project";
        const auto outsideDirectory = temporary.Root / "project-sibling";
        fs::create_directories(projectRoot / "assets");
        fs::create_directory(outsideDirectory);

        TestValidProjects(runner, projectRoot, outsideDirectory);
        TestInvalidProjects(runner, projectRoot, outsideDirectory);
        TestProjectCreation(runner, temporary.Root, outsideDirectory);
        std::cout << "ProjectConfiguration: " << runner.Checks << " checks, "
                  << runner.Failures << " failures\n";
        return runner.Failures == 0 ? 0 : 1;
    }
    catch (const std::exception &exception)
    {
        std::cerr << "ProjectConfiguration test setup failed: " << exception.what() << '\n';
        return 1;
    }
}
