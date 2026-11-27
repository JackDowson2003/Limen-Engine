#pragma once

#include <filesystem>

namespace Limen
{
    class AssetRegistry;
    struct ProjectConfiguration;
    struct ProjectConfigurationError;
}

namespace LimenEditor
{
    // 使用已加载的项目与注册表解析启动场景。未配置时返回空路径，失败时保留 output。
    [[nodiscard]] bool TryResolveEditorStartupScene(
        const Limen::ProjectConfiguration &project,
        const Limen::AssetRegistry &registry,
        std::filesystem::path &output,
        Limen::ProjectConfigurationError &error
    );
}
