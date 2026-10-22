//
// Created by chenlong on 2026/10/20.
//
#include <filesystem>
#include <mach-o/dyld.h>
#include <cstdint>
#include <system_error>
#include "Limen/Platform/PlatformPaths.h"
#include "Limen/Tool/LString.h"
#include "Limen/Tool/LArray.h"

namespace Limen
{
#ifdef LIMEN_PLATFORM_MACOS
    bool TryGetBuiltInAssetRoot(std::filesystem::path &output, LString &error)
    {
        std::uint32_t pathSize = 1024;
        LArray<char> rawPath(pathSize);

        if (_NSGetExecutablePath(rawPath.getData(), &pathSize) != 0)
        {
            if (pathSize <= rawPath.getSize())
            {
                error = "Cannot determine executable path size";
                return false;
            }

            rawPath = LArray<char>(pathSize);
            if (_NSGetExecutablePath(rawPath.getData(), &pathSize) != 0)
            {
                error = "Cannot obtain executable path";
                return false;
            }
        }

        std::error_code fsError;
        // canonical 同时把可能的符号链接解析到实际可执行文件。
        const auto executable = std::filesystem::canonical(
            std::filesystem::path(rawPath.getData()), fsError);
        if (fsError)
        {
            error = "Cannot resolve executable path";
            return false;
        }

        const auto assetRoot = std::filesystem::canonical(
            executable.parent_path() / "assets", fsError);
        if (fsError || !std::filesystem::is_directory(assetRoot, fsError))
        {
            error = "Built-in assets directory is missing";
            return false;
        }

        output = assetRoot;
        error.clear();
        return true;
    }
#endif
}
