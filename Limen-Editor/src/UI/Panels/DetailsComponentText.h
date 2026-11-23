#pragma once

#include <string>
#include <string_view>

namespace Limen
{
    struct SceneObjectComponent;
}

namespace LimenEditor
{
    struct DetailsComponentDescription
    {
        std::string Title;
        std::string Summary;
        std::string Detail;
    };

    // Model 显示名优先使用项目内逻辑资产路径；未解析到时回退到 AssetID。
    [[nodiscard]] DetailsComponentDescription
    DescribeSceneObjectComponent(
        const Limen::SceneObjectComponent &component,
        std::string_view modelAssetDisplayName = {}
    );
}
