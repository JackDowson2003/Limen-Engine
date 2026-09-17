//
// Created by chenlong on 2026/9/16.
//

#include "Limen/Asset/AssetPath.h"

#include "Limen/Core/Log.h"

namespace Limen
{
    std::filesystem::path AssetPath::s_Root{"assets"};

    void AssetPath::SetRoot(const std::filesystem::path &root)
    {
        /*
         * 空路径通常表示调用方配置错误。
         * 保留之前的根目录，避免后续路径全部解析失败。
         */
        if (root.empty())
        {
            LM_CORE_WARN(
                "Cannot set an empty asset root; "
                "keeping current root '{}'",
                s_Root.string()
            );

            return;
        }

        /*
         * lexically_normal只整理路径字符串，
         * 不要求目标文件或目录已经存在。
         *
         * 例如：
         * assets/./models/../textures
         * 会整理为：
         * assets/textures
         */
        s_Root = root.lexically_normal();
    }

    const std::filesystem::path & AssetPath::GetRoot() noexcept
    {
        return s_Root;
    }

    std::filesystem::path AssetPath::Resolve(const std::filesystem::path &logicalPath)
    {
        if (logicalPath.empty())
            return {};

        // 如果是逻辑路径进行拼接
        if (logicalPath.is_absolute())
            return logicalPath.lexically_normal();

        /*
         * 相对逻辑路径拼接到资源根目录。
         *
         * 例如：
         * Root = "assets"
         * logicalPath = "models/TestCube.obj"
         *
         * 结果：
         * "assets/models/TestCube.obj"
         */
        return (s_Root / logicalPath).lexically_normal();
    }
}
