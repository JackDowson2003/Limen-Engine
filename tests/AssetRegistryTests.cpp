#include "Limen/Asset/AssetRegistry.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace
{
    namespace fs = std::filesystem;

    constexpr Limen::AssetID kKnownID{0xd0360f3def2e41d3ULL, 0xa0b69bcc5940a539ULL};
    constexpr Limen::AssetID kOtherID{0x0011223344556677ULL, 0x8899aabbccddeeffULL};
    constexpr Limen::AssetID kSentinelID{0x123456789abcdef0ULL, 0x0fedcba987654321ULL};

    struct TestRunner
    {
        int Failures = 0;

        void Check(const std::string_view name, const bool passed)
        {
            if (passed)
                return;
            ++Failures;
            std::cerr << "FAIL: " << name << '\n';
        }
    };

    // 用例仅清理自己独占创建的目录，不依赖仓库的当前工作目录。
    struct TemporaryProject
    {
        fs::path Root;
        fs::path Assets;

        TemporaryProject()
        {
            const auto base = fs::temp_directory_path();
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            for (int attempt = 0; attempt < 100; ++attempt)
            {
                const auto candidate = base / ("limen-asset-registry-test-" +
                                               std::to_string(stamp) + "-" + std::to_string(attempt));
                std::error_code error;
                if (fs::create_directory(candidate, error))
                {
                    Root = candidate;
                    Assets = Root / "assets";
                    fs::create_directory(Assets);
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

        TemporaryProject(const TemporaryProject&) = delete;
        TemporaryProject& operator=(const TemporaryProject&) = delete;
    };

    struct ScopedCurrentPath
    {
        fs::path Previous = fs::current_path();

        explicit ScopedCurrentPath(const fs::path& next)
        {
            fs::current_path(next);
        }

        ~ScopedCurrentPath()
        {
            std::error_code ignored;
            fs::current_path(Previous, ignored);
        }

        ScopedCurrentPath(const ScopedCurrentPath&) = delete;
        ScopedCurrentPath& operator=(const ScopedCurrentPath&) = delete;
    };

    void WriteFile(const fs::path& path, const std::string_view text)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file)
            throw std::runtime_error("Cannot open test asset file");
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!file)
            throw std::runtime_error("Cannot write test asset file");
    }

    std::string SidecarText(const Limen::AssetID id, const int schemaVersion = 1)
    {
        const auto idText = Limen::AssetIDToString(id);
        return "schema_version = " + std::to_string(schemaVersion) + "\nasset_id = \"" +
               std::string(idText.data(), idText.size()) + "\"\n";
    }

    void CheckRecord(TestRunner& runner, const std::string_view label,
                     Limen::AssetRegistry& registry, const Limen::AssetID id,
                     const std::string_view logicalPath, const Limen::AssetRecordStatus status)
    {
        Limen::AssetRecord record;
        runner.Check(std::string(label) + ": found by ID", registry.TryFind(id, record));
        runner.Check(std::string(label) + ": record fields",
                     record.ID == id && static_cast<std::string_view>(record.LogicalPath) == logicalPath &&
                     record.Status == status);

        auto foundID = kSentinelID;
        runner.Check(std::string(label) + ": found by path", registry.TryFindID(logicalPath, foundID));
        runner.Check(std::string(label) + ": path resolves to same ID", foundID == id);
    }

    void TestRegisterReopenAndMissingSource(TestRunner& runner)
    {
        TemporaryProject project;
        fs::create_directory(project.Assets / "Textures");
        fs::create_directory(project.Root / "elsewhere");
        WriteFile(project.Assets / "Textures" / "brick.png", "asset bytes");

        Limen::AssetID registered{};
        {
            Limen::AssetRegistry registry(project.Assets);
            Limen::AssetRegistryError error;
            auto output = kSentinelID;
            runner.Check("register before load rejected", !registry.TryRegister("Textures/brick.png", output, error));
            runner.Check("register before load reports NotLoaded", error.Code == Limen::AssetRegistryErrorCode::NotLoaded);
            runner.Check("register before load preserves output", output == kSentinelID);
            runner.Check("register before load creates no sidecar",
                         !fs::exists(project.Assets / "Textures" / "brick.png.lmmeta"));

            runner.Check("empty registry loads", registry.TryLoad(error));
            runner.Check("successful load clears error", error.Code == Limen::AssetRegistryErrorCode::None);

            output = kSentinelID;
            runner.Check("unknown path not found", !registry.TryFindID("Textures/brick.png", output));
            runner.Check("unknown path preserves output", output == kSentinelID);

            runner.Check("register source", registry.TryRegister("Textures/brick.png", registered, error));
            runner.Check("register generates valid ID", registered.isValid());
            runner.Check("register clears error", error.Code == Limen::AssetRegistryErrorCode::None);
            runner.Check("register creates sidecar", fs::is_regular_file(project.Assets / "Textures" / "brick.png.lmmeta"));
            bool unexpectedFile = false;
            for (const auto& entry : fs::directory_iterator(project.Assets / "Textures"))
            {
                const auto name = entry.path().filename();
                if (name != "brick.png" && name != "brick.png.lmmeta")
                    unexpectedFile = true;
            }
            runner.Check("successful registration removes staging files", !unexpectedFile);
            CheckRecord(runner, "registered source", registry, registered,
                        "Textures/brick.png", Limen::AssetRecordStatus::Present);

            output = kSentinelID;
            runner.Check("duplicate registration rejected", !registry.TryRegister("Textures/brick.png", output, error));
            runner.Check("duplicate registration reports AlreadyRegistered",
                         error.Code == Limen::AssetRegistryErrorCode::AlreadyRegistered);
            runner.Check("duplicate registration preserves output", output == kSentinelID);

            Limen::AssetRecord missingOutput{
                .ID = kSentinelID, .LogicalPath = "keep-me", .Status = Limen::AssetRecordStatus::Present};
            runner.Check("unknown ID not found", !registry.TryFind(kOtherID, missingOutput));
            runner.Check("unknown ID preserves output",
                         missingOutput.ID == kSentinelID && missingOutput.LogicalPath == "keep-me" &&
                         missingOutput.Status == Limen::AssetRecordStatus::Present);
        }

        {
            // 持久身份只来自 sidecar，不能依赖原对象或进程的当前目录。
            ScopedCurrentPath away(project.Root / "elsewhere");
            Limen::AssetRegistry reopened(project.Assets);
            Limen::AssetRegistryError error;
            runner.Check("reopen from other working directory", reopened.TryLoad(error));
            CheckRecord(runner, "reopened source", reopened, registered,
                        "Textures/brick.png", Limen::AssetRecordStatus::Present);
        }

        fs::remove(project.Assets / "Textures" / "brick.png");
        Limen::AssetRegistry missing(project.Assets);
        Limen::AssetRegistryError error;
        runner.Check("load after source removal", missing.TryLoad(error));
        CheckRecord(runner, "source removed but identity retained", missing, registered,
                    "Textures/brick.png", Limen::AssetRecordStatus::Missing);
    }

    void TestFailedReloadPreservesPublishedIndex(TestRunner& runner)
    {
        TemporaryProject project;
        WriteFile(project.Assets / "known.bin", "asset bytes");
        WriteFile(project.Assets / "known.bin.lmmeta", SidecarText(kKnownID));

        Limen::AssetRegistry registry(project.Assets);
        Limen::AssetRegistryError error;
        runner.Check("known sidecar loads", registry.TryLoad(error));
        CheckRecord(runner, "known record", registry, kKnownID,
                    "known.bin", Limen::AssetRecordStatus::Present);

        auto checkFailedReload = [&](const std::string_view label, const fs::path& sidecar,
                                     const std::string_view text,
                                     const Limen::AssetRegistryErrorCode expectedCode)
        {
            WriteFile(sidecar, text);
            const bool loaded = registry.TryLoad(error);
            runner.Check(std::string(label) + ": reload rejected", !loaded);
            runner.Check(std::string(label) + ": error code", error.Code == expectedCode);
            // macOS 的 /var 与 /private/var 可指向同一文件，扫描器会返回规范化后的路径。
            std::error_code pathError;
            bool expectedPath = fs::equivalent(error.Path, sidecar, pathError) && !pathError;
            if (expectedCode == Limen::AssetRegistryErrorCode::DuplicateID && !expectedPath)
            {
                // 重复 ID 报告的是扫描到的第二个文件，目录遍历顺序没有保证。
                std::error_code otherPathError;
                expectedPath = fs::equivalent(error.Path, project.Assets / "known.bin.lmmeta",
                                              otherPathError) && !otherPathError;
            }
            runner.Check(std::string(label) + ": error path", expectedPath);
            CheckRecord(runner, std::string(label) + ": old index retained", registry,
                        kKnownID, "known.bin", Limen::AssetRecordStatus::Present);
            fs::remove(sidecar);
        };

        checkFailedReload("malformed metadata", project.Assets / "bad.bin.lmmeta",
                          "schema_version = 1\nasset_id = [\n",
                          Limen::AssetRegistryErrorCode::InvalidMetadata);
        checkFailedReload("unsupported schema", project.Assets / "future.bin.lmmeta",
                          SidecarText(kOtherID, 2), Limen::AssetRegistryErrorCode::UnsupportedSchema);
        checkFailedReload("duplicate persistent ID", project.Assets / "duplicate.bin.lmmeta",
                          SidecarText(kKnownID), Limen::AssetRegistryErrorCode::DuplicateID);
    }

    void ExpectRegisterFailure(TestRunner& runner, Limen::AssetRegistry& registry,
                               const std::string_view label, const std::string_view path,
                               const Limen::AssetRegistryErrorCode expectedCode)
    {
        auto output = kSentinelID;
        Limen::AssetRegistryError error;
        bool registered = false;
        bool threw = false;
        try
        {
            registered = registry.TryRegister(path, output, error);
        }
        catch (const std::exception&)
        {
            threw = true;
        }
        runner.Check(std::string(label) + ": no exception", !threw);
        if (threw)
            return;
        runner.Check(std::string(label) + ": rejected", !registered);
        runner.Check(std::string(label) + ": error code", error.Code == expectedCode);
        runner.Check(std::string(label) + ": output unchanged", output == kSentinelID);
    }

    void TestRegistrationPathSafety(TestRunner& runner)
    {
        TemporaryProject project;
        WriteFile(project.Assets / "Case.bin", "asset bytes");
        WriteFile(project.Assets / "occupied.bin", "asset bytes");
        WriteFile(project.Assets / "variant.bin", "asset bytes");
        WriteFile(project.Root / "outside.bin", "outside assets");

        Limen::AssetRegistry registry(project.Assets);
        Limen::AssetRegistryError error;
        runner.Check("path safety fixture loads", registry.TryLoad(error));

        for (const auto path: {"", "../outside.bin", "a//b", "a/./b", "/Case.bin",
                              "a\\b", "Case.bin/", "Case.bin.lmmeta", "CON.txt"})
        {
            ExpectRegisterFailure(runner, registry, std::string("invalid path '") + path + "'",
                                  path, Limen::AssetRegistryErrorCode::InvalidPath);
        }
        ExpectRegisterFailure(runner, registry, "missing source", "missing.bin",
                              Limen::AssetRegistryErrorCode::MissingAsset);
        ExpectRegisterFailure(runner, registry, "case mismatch", "case.bin",
                              Limen::AssetRegistryErrorCode::InvalidPath);

        // 逻辑路径契约是 UTF-8；无效字节必须在 filesystem 转换前拒绝。
        const std::string invalidUtf8("\xC3\x28", 2);
        ExpectRegisterFailure(runner, registry, "invalid UTF-8", invalidUtf8,
                              Limen::AssetRegistryErrorCode::InvalidPath);

        std::error_code linkError;
        fs::create_symlink(project.Root / "outside.bin", project.Assets / "escape.bin", linkError);
        if (!linkError)
            ExpectRegisterFailure(runner, registry, "source symlink escapes root", "escape.bin",
                                  Limen::AssetRegistryErrorCode::InvalidPath);
        else
            std::cout << "SKIP: source symlink fixture (" << linkError.message() << ")\n";

        // 占位 sidecar 在 TryLoad 之后出现，验证注册时仍拒绝覆盖磁盘内容。
        WriteFile(project.Assets / "occupied.bin.lmmeta", "do not overwrite");
        ExpectRegisterFailure(runner, registry, "existing sidecar", "occupied.bin",
                              Limen::AssetRegistryErrorCode::AlreadyRegistered);

        WriteFile(project.Assets / "variant.bin.LMMETA", "case variant");
        ExpectRegisterFailure(runner, registry, "sidecar case variant", "variant.bin",
                              Limen::AssetRegistryErrorCode::DuplicatePath);
    }

    void TestSidecarCannotDescribeAnotherSidecar(TestRunner& runner)
    {
        TemporaryProject project;
        // 两个文件都写成有效 TOML，隔离“源资产后缀非法”这一规则，避免解析错误掩盖它。
        WriteFile(project.Assets / "nested.lmmeta", SidecarText(kKnownID));
        WriteFile(project.Assets / "nested.lmmeta.lmmeta", SidecarText(kOtherID));

        Limen::AssetRegistry registry(project.Assets);
        Limen::AssetRegistryError error;
        runner.Check("sidecar-derived .lmmeta source rejected", !registry.TryLoad(error));
        runner.Check("sidecar-derived .lmmeta reports InvalidPath",
                     error.Code == Limen::AssetRegistryErrorCode::InvalidPath);
        Limen::AssetRecord output;
        runner.Check("failed initial load publishes no records",
                     !registry.TryFind(kKnownID, output));
    }

    void TestSidecarSuffixCase(TestRunner& runner)
    {
        TemporaryProject project;
        WriteFile(project.Assets / "case.bin", "asset bytes");
        const auto sidecar = project.Assets / "case.bin.LMMETA";
        WriteFile(sidecar, SidecarText(kKnownID));

        Limen::AssetRegistry registry(project.Assets);
        Limen::AssetRegistryError error;
        runner.Check("uppercase sidecar suffix rejected", !registry.TryLoad(error));
        runner.Check("uppercase sidecar suffix reports InvalidPath",
                     error.Code == Limen::AssetRegistryErrorCode::InvalidPath);
        std::error_code pathError;
        runner.Check("uppercase sidecar suffix reports offending file",
                     fs::equivalent(error.Path, sidecar, pathError) && !pathError);

        Limen::AssetRecord output;
        runner.Check("uppercase sidecar failure publishes no records",
                     !registry.TryFind(kKnownID, output));
    }
}

int main()
{
    TestRunner runner;
    try
    {
        TestRegisterReopenAndMissingSource(runner);
        TestFailedReloadPreservesPublishedIndex(runner);
        TestRegistrationPathSafety(runner);
        TestSidecarCannotDescribeAnotherSidecar(runner);
        TestSidecarSuffixCase(runner);
    }
    catch (const std::exception& error)
    {
        ++runner.Failures;
        std::cerr << "FAIL: unexpected test setup exception: " << error.what() << '\n';
    }
    return runner.Failures == 0 ? 0 : 1;
}
