#include "UI/EditorUILayout.h"

#include <algorithm>
#include <cmath>

namespace LimenEditor
{
    EditorUILayoutResult CalculateEditorUILayout(
        const float logicalWidth,
        const float logicalHeight,
        const float contentHeightRatio,
        const bool showContent,
        const bool viewportExpanded,
        const float rightSplitRatio
    )
    {
        EditorUILayoutResult result{};

        // 最小化或尺寸无效时，所有区域保持为空，避免沿用上一帧命中范围。
        if (!std::isfinite(logicalWidth) || !std::isfinite(logicalHeight) ||
            logicalWidth <= 0.0f || logicalHeight <= 0.0f)
            return result;

        constexpr float kCompactTopHeight = 62.0f;
        constexpr float kStackedTopHeight = 92.0f;
        constexpr float kStatusHeight = 30.0f;
        constexpr float kMinViewportWidth = 400.0f;
        constexpr float kViewportToolbarHeight = 35.0f;
        constexpr float kMinViewportHeightWithToolbar = 32.0f;

        const float preferredTopHeight = logicalWidth >= kEditorWideToolbarWidth
            ? kCompactTopHeight : kStackedTopHeight;
        const float statusHeight = std::min(kStatusHeight, logicalHeight);
        const float statusY = logicalHeight - statusHeight;
        const float topHeight = std::min(preferredTopHeight, statusY);
        result.Top = {0.0f, 0.0f, logicalWidth, topHeight};
        result.Status = {0.0f, statusY, logicalWidth, statusHeight};

        const float bodyHeight = statusY - topHeight;
        if (bodyHeight <= 0.0f)
            return result;

        constexpr float kRightColumnRatio = 0.24f;
        constexpr float kMinRightWidth = 270.0f;
        constexpr float kMaxRightWidth = 405.0f;
        constexpr float kMinUsableRightWidth = 160.0f;
        constexpr float kMinPanelHeight = 78.0f;
        constexpr float kDetailsSplitterHeight = 4.0f;

        const float availableRightWidth = logicalWidth - kMinViewportWidth;
        const bool showRightColumn = !viewportExpanded && availableRightWidth >= kMinUsableRightWidth &&
                                     bodyHeight >= kMinPanelHeight * 2.0f + kDetailsSplitterHeight;
        const float rightWidth = showRightColumn
            ? std::min(availableRightWidth, std::clamp(logicalWidth * kRightColumnRatio, kMinRightWidth, kMaxRightWidth))
            : 0.0f;
        const float viewportWidth = logicalWidth - rightWidth;
        if (showRightColumn)
        {
            const float availableHeight = bodyHeight - kDetailsSplitterHeight;
            const float ratio = std::isfinite(rightSplitRatio)
                ? std::clamp(rightSplitRatio, 0.0f, 1.0f) : kDefaultEditorRightSplitRatio;
            // 右侧纵向分割独立于资源抽屉，任一极端拖动仍给两块面板留下空间。
            const float outlinerHeight = std::clamp(availableHeight * ratio,
                                                    kMinPanelHeight, availableHeight - kMinPanelHeight);
            const float splitterY = topHeight + outlinerHeight;
            result.Outliner = {viewportWidth, topHeight, rightWidth, outlinerHeight};
            result.DetailsSplitter = {viewportWidth, splitterY, rightWidth, kDetailsSplitterHeight};
            result.Inspector = {viewportWidth, splitterY + kDetailsSplitterHeight, rightWidth,
                                availableHeight - outlinerHeight};
        }

        float viewportBottom = statusY;
        if (showContent && !viewportExpanded)
        {
            constexpr float kContentSplitterHeight = 4.0f;
            constexpr float kMinOpenContentHeight = 125.0f;
            constexpr float kMinUsefulContentHeight = 112.0f;
            constexpr float kMinViewportBodyHeight = 150.0f + kViewportToolbarHeight;
            constexpr float kMaxContentHeightRatio = 0.55f;
            constexpr float kDefaultContentHeightRatio = 0.24f;

            // 视口保留导航空间，抽屉至少容得下标题和资源卡片的主要内容。
            const float availableHeight = bodyHeight - kContentSplitterHeight;
            const float maxContentHeight = std::min(availableHeight - kMinViewportBodyHeight,
                                                    availableHeight * kMaxContentHeightRatio);
            if (maxContentHeight >= kMinUsefulContentHeight)
            {
                const float ratio = std::isfinite(contentHeightRatio)
                    ? std::clamp(contentHeightRatio, 0.0f, 1.0f) : kDefaultContentHeightRatio;
                const float minContentHeight = std::min(kMinOpenContentHeight, maxContentHeight);
                const float height = std::clamp(availableHeight * ratio, minContentHeight, maxContentHeight);
                const float y = statusY - height;
                viewportBottom = y - kContentSplitterHeight;
                result.ContentSplitter = {0.0f, viewportBottom, viewportWidth, kContentSplitterHeight};
                result.Content = {0.0f, y, viewportWidth, height};
            }
        }

        const float viewportBodyHeight = viewportBottom - topHeight;
        const float viewportToolbarHeight = viewportBodyHeight >= kViewportToolbarHeight + kMinViewportHeightWithToolbar
            ? kViewportToolbarHeight : 0.0f;
        result.ViewportToolbar = {0.0f, topHeight, viewportWidth, viewportToolbarHeight};
        result.Center = {0.0f, topHeight + viewportToolbarHeight, viewportWidth,
                         viewportBodyHeight - viewportToolbarHeight};

        return result;
    }

    ViewportPixelSize CalculateViewportPixelSize(
        const float logicalWindowWidth,
        const float logicalWindowHeight,
        const UIRect &logicalRect,
        const uint32_t framebufferWidth,
        const uint32_t framebufferHeight)
    {
        ViewportPixelSize res{};

        // 任一输入无效时返回最小 1x1, 避免视口尺寸为0
        if (logicalWindowWidth <= 0.0f || logicalWindowHeight <= 0.0f ||
            framebufferWidth == 0 || framebufferHeight == 0 ||
            logicalRect.Width <= 0.0f || logicalRect.Height <= 0.0f
        )
        {
            return res;
        }

        const float scaleX = static_cast<float>(framebufferWidth) / logicalWindowWidth;
        const float scaleY = static_cast<float>(framebufferHeight) / logicalWindowHeight;

        const long width = std::lround(logicalRect.Width * scaleX);
        const long height = std::lround(logicalRect.Height * scaleY);

        res.Width = static_cast<uint32_t>(std::max(1L, width));
        res.Height = static_cast<uint32_t>(std::max(1L, height));
        return res;
    }

    int FindListRowAt(const float firstRowY, const float rowHeight, const float mouseY,
                      const uint32_t rowCount) noexcept
    {
        if (rowHeight <= 0.0f || rowCount == 0 || mouseY < firstRowY)
            return -1;

        const int row = static_cast<int>((mouseY - firstRowY) / rowHeight);
        return row >= 0 && row < static_cast<int>(rowCount) ? row : -1;
    }

    Limen::LArray<Limen::SceneObjectID> BuildSelectionRange(
        const Limen::LArray<Limen::SceneObjectID> &orderedRows,
        const Limen::SceneObjectID anchor,
        const Limen::SceneObjectID target)
    {
        Limen::LArray<Limen::SceneObjectID> result;

        // anchor 是上一次普通点击/增选的行；target 是本次 Shift+点击的行。
        // 两者都必须在当前 Outliner 行顺序中存在，范围才有效。
        int anchorIndex = -1;
        int targetIndex = -1;

        for (uint32_t i = 0; i < orderedRows.getSize(); ++i)
        {
            if (orderedRows[i] == anchor)
                anchorIndex = static_cast<int>(i);
            if (orderedRows[i] == target)
                targetIndex = static_cast<int>(i);
        }

        // 目标行都不存在时没有可选范围。
        if (targetIndex < 0)
            return result;

        // 锚点失效（例如对象刚被删除）时退化为只选当前行。
        if (anchorIndex < 0)
        {
            result.push_back(target);
            return result;
        }

        // 无论向上还是向下拖选，结果都按 Outliner 行顺序从前往后返回。
        const int first = std::min(anchorIndex, targetIndex);
        const int last = std::max(anchorIndex, targetIndex);
        for (int i = first; i <= last; ++i)
            result.push_back(orderedRows[static_cast<uint32_t>(i)]);

        return result;
    }
}
