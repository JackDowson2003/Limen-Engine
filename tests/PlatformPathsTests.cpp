#include <filesystem>
#include <iostream>
#include <string_view>
#include <system_error>

#include "Limen/Platform/PlatformPaths.h"

namespace
{
    namespace fs = std::filesystem;

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

    // 测试只删除自己创建的空目录；不清理已有目录或其内容。
    struct ScopedAssetDirectory
    {
        fs::path Path;
        bool Created = false;

        ~ScopedAssetDirectory()
        {
            if (Created)
            {
                std::error_code ignored;
                fs::remove(Path, ignored);
            }
        }
    };

    struct ScopedWorkingDirectory
    {
        fs::path Original = fs::current_path();

        ~ScopedWorkingDirectory()
        {
            std::error_code ignored;
            fs::current_path(Original, ignored);
        }
    };
}

int main(int argc, char **argv)
{
    if (argc < 1 || argv[0] == nullptr)
    {
        std::cerr << "Cannot determine test executable path\n";
        return 2;
    }

    std::error_code fsError;
    const fs::path executable = fs::canonical(fs::path(argv[0]), fsError);
    if (fsError)
    {
        std::cerr << "Cannot resolve test executable: " << fsError.message() << '\n';
        return 2;
    }

    // CMake 把测试可执行文件放在独立目录，避免触及其他测试的资源。
    ScopedAssetDirectory assets{executable.parent_path() / "assets"};
    if (fs::exists(assets.Path, fsError) || fsError)
    {
        std::cerr << "Test asset directory already exists: " << assets.Path << '\n';
        return 2;
    }

    TestRunner runner;
    fs::path output = "unchanged-on-failure";
    Limen::LString error = "stale error";

    runner.Check("missing assets fails",
                 !Limen::TryGetBuiltInAssetRoot(output, error));
    runner.Check("failure preserves output",
                 output == fs::path("unchanged-on-failure"));
    runner.Check("failure reports an error", !error.empty());

    assets.Created = fs::create_directory(assets.Path, fsError);
    if (!assets.Created || fsError)
    {
        std::cerr << "Cannot create test asset directory: " << fsError.message() << '\n';
        return 2;
    }

    output = "unchanged-on-success";
    error = "stale error";
    {
        ScopedWorkingDirectory workingDirectory;
        // 资源根必须跟随可执行文件，而不是受调用方当前工作目录影响。
        fs::current_path(fs::temp_directory_path());
        runner.Check("finds assets from unrelated working directory",
                     Limen::TryGetBuiltInAssetRoot(output, error));
    }

    runner.Check("returns canonical absolute asset directory",
                 output == fs::canonical(assets.Path) && output.is_absolute());
    runner.Check("success clears previous error", error.empty());

    std::cout << "PlatformPaths: " << runner.Checks << " checks, "
              << runner.Failures << " failures\n";
    return runner.Failures == 0 ? 0 : 1;
}
