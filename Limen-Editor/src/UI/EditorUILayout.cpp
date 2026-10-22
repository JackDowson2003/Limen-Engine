#include "UI/EditorUILayout.h"

#include <algorithm>

namespace LimenEditor
{
    EditorUILayoutResult CalculateEditorUILayout(
        const float logicalWidth,
        const float logicalHeight,
        const float rightTopRatio
    )
    {
        EditorUILayoutResult result{};

        // 最小化或尺寸无效时，所有区域保持为空，避免沿用上一帧命中范围。
        if (logicalWidth <= 0.0f || logicalHeight <= 0.0f)
            return result;

        constexpr float kTopHeight = 128.0f;
        constexpr float kMiddlePreferredPercent = 0.65f;
        constexpr float kMinMiddleWidth = 240.0f;
        constexpr float kSplitterHeight = 4.0f;

        const float topHeight = std::min(kTopHeight, logicalHeight);
        result.Top =
        {
            .X = 0.0f,
            .Y = 0.0f,
            .Width = logicalWidth,
            .Height = topHeight
        };

        const float bodyHeight = logicalHeight - topHeight;
        if (bodyHeight <= 0.0f)
            return result;

        const float middleWidth = std::min(
            logicalWidth,
            std::max(kMinMiddleWidth, logicalWidth * kMiddlePreferredPercent)
        );
        const float sideWidth = (logicalWidth - middleWidth) * 0.5f;
        const float rightX = sideWidth + middleWidth;

        result.Left =
        {
            .X = 0.0f,
            .Y = topHeight,
            .Width = sideWidth,
            .Height = bodyHeight
        };
        result.Center =
        {
            .X = sideWidth,
            .Y = topHeight,
            .Width = middleWidth,
            .Height = bodyHeight
        };

        // 分隔条先占用高度，剩余高度才按比例分给右上、右下区域。
        const float splitterHeight = std::min(kSplitterHeight, bodyHeight);
        const float availableHeight = bodyHeight - splitterHeight;
        const float rightTopHeight = availableHeight * rightTopRatio;
        const float rightBottomHeight = availableHeight - rightTopHeight;
        const float splitterY = topHeight + rightTopHeight;

        result.RightTop =
        {
            .X = rightX,
            .Y = topHeight,
            .Width = sideWidth,
            .Height = rightTopHeight
        };
        result.RightSplitter =
        {
            .X = rightX,
            .Y = splitterY,
            .Width = sideWidth,
            .Height = splitterHeight
        };
        result.RightBottom = {
            .X = rightX,
            .Y = splitterY + splitterHeight,
            .Width = sideWidth,
            .Height = rightBottomHeight
        };


        return result;
    }
}
