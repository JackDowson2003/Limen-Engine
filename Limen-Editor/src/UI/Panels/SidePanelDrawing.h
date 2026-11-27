#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

#include "Limen/Renderer/FontAtlas.h"
#include "Render/EditorUIRenderer.h"
#include "UI/EditorUILayout.h"
#include "UI/EditorTheme.h"

namespace LimenEditor::SidePanelDrawing
{
    inline void DrawText(
        const Limen::Window &window,
        const EditorUIRenderer &renderer,
        const Limen::FontAtlas &fontAtlas,
        const glm::vec2 &position,
        const glm::vec4 &color,
        const float width,
        const std::string_view text)
    {
        if (width <= 0.0f || text.empty())
            return;

        if (renderer.MeasureTextWidth(fontAtlas, text) <= width)
        {
            renderer.DrawText(window, position, color, fontAtlas, text);
            return;
        }

        constexpr std::string_view ellipsis = "...";
        const float prefixWidth = width - renderer.MeasureTextWidth(fontAtlas, ellipsis);
        if (prefixWidth < 0.0f)
            return;

        // 缩短可见文本时对齐 UTF-8 字符边界，避免截断多字节字符。
        const auto characterBoundary = [text](std::size_t offset)
        {
            while (offset > 0 && offset < text.size() &&
                   (static_cast<unsigned char>(text[offset]) & 0xc0U) == 0x80U)
                --offset;
            return offset;
        };

        std::size_t first = 0;
        std::size_t last = text.size();
        while (first < last)
        {
            const auto middle = first + (last - first + 1) / 2;
            if (renderer.MeasureTextWidth(fontAtlas, text.substr(0, characterBoundary(middle))) <= prefixWidth)
                first = middle;
            else
                last = middle - 1;
        }

        std::string shortened(text.substr(0, characterBoundary(first)));
        shortened += ellipsis;
        renderer.DrawText(window, position, color, fontAtlas, shortened);
    }

    inline void DrawHeader(
        const Limen::Window &window,
        const EditorUIRenderer &renderer,
        const Limen::FontAtlas &fontAtlas,
        const EditorTheme &theme,
        const UIRect &rect,
        const std::string_view title,
        const float reservedRight = 0.0f)
    {
        const float padding = theme.PanelPadding;
        const float contentWidth = rect.Width - padding * 2.0f;
        const float textY = rect.Y + std::max(0.0f, (theme.PanelHeaderHeight - fontAtlas.GetLineHeight()) * 0.5f);
        DrawText(window, renderer, fontAtlas, {rect.X + padding, textY},
                 theme.TextPrimary, contentWidth - reservedRight, title);

        if (contentWidth > 0.0f && rect.Height >= theme.PanelHeaderHeight)
            renderer.DrawSolidRect(window, {rect.X + padding, rect.Y + theme.PanelHeaderHeight - 1.0f},
                                   {contentWidth, 1.0f}, theme.PanelBorder);
    }
}
