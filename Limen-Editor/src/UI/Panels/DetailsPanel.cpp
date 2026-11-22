//
// Created by chenlong on 2026/11/21.
//
#include "UI/Panels/DetailsPanel.h"

#include "Limen/Application/Window.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Project/EditorSession.h"
#include "Render/EditorUIRenderer.h"
#include "UI/EditorTheme.h"

namespace LimenEditor
{
    namespace
    {
        std::string FormatPosition(const glm::mat4 &transform)
        {
            std::string position = "Pos ";
            position += std::to_string(transform[3][0]);
            position += ", ";
            position += std::to_string(transform[3][1]);
            position += ", ";
            position += std::to_string(transform[3][2]);
            return position;
        }
    }

    void DetailsPanel::Draw(
        const EditorSession &session,
        const Limen::Window &window,
        const EditorUIRenderer &renderer,
        const Limen::FontAtlas &fontAtlas,
        const EditorTheme &theme,
        const UIRect &rect) const
    {
        const float textX = rect.X + theme.PanelPadding;
        const float headerY = rect.Y + theme.HeaderOffset;
        const float rowHeight = fontAtlas.GetLineHeight() + theme.RowSpacing;

        // 面板内容只在自身矩形内绘制，避免覆盖右侧其它区域。
        renderer.SetClipRect(
            window,
            {rect.X, rect.Y},
            {rect.Width, rect.Height}
        );

        renderer.DrawText(
            window,
            {textX, headerY},
            theme.TextPrimary,
            fontAtlas,
            "Details"
        );

        // ReSharper disable once CppTooWideScopeInitStatement
        const uint32_t selectedCount = session.GetSelectedObjectCount();
        if (selectedCount == 0)
        {
            // 空选择：明确显示没有对象被选中。
            renderer.DrawText(
                window,
                {textX, headerY + rowHeight},
                theme.TextMuted,
                fontAtlas,
                "No selection"
            );
        } else if (selectedCount > 1)
        {
            // 多选先显示数量，再显示公共 Transform 或 Multiple Values。
            renderer.DrawText(
                window,
                {textX, headerY + rowHeight},
                theme.TextSecondary,
                fontAtlas,
                std::to_string(selectedCount) + " Objects selected");

            const Limen::LArray<Limen::SceneObjectID> &selected =
                session.GetSelectedObjectIDs();

            glm::mat4 first{1.0f};
            bool available = selected.getSize() > 0 &&
                             session.TryGetObjectTransform(selected[0], first);
            bool sameTransform = available;

            for (uint32_t i = 1; available && i < selected.getSize(); ++i)
            {
                glm::mat4 other{1.0f};
                if (!session.TryGetObjectTransform(selected[i], other))
                {
                    available = false;
                    break;
                }
                if (!(other == first))
                    sameTransform = false;
            }

            if (!available)
            {
                renderer.DrawText(
                    window,
                    {textX, headerY + rowHeight * 2.0f},
                    theme.MissingText,
                    fontAtlas,
                    "Transform unavailable");
            }
            else
            {
                renderer.DrawText(
                    window,
                    {textX, headerY + rowHeight * 2.0f},
                    theme.TextSecondary,
                    fontAtlas,
                    sameTransform
                        ? FormatPosition(first)
                        : "Pos Multiple Values");
            }
        } else
        {
            // 单选：显示当前对象的平移分量。
            glm::mat4 transform{1.0f};
            if (session.TryGetSelectedObjectTransform(transform))
            {
                renderer.DrawText(
                    window,
                    {textX, headerY + rowHeight},
                    theme.TextSecondary,
                    fontAtlas,
                    FormatPosition(transform));
            } else
            {
                renderer.DrawText(
                    window,
                    {textX, headerY + rowHeight},
                    theme.MissingText,
                    fontAtlas,
                    "Transform unavailable");
            }
        }

        // 恢复裁剪，避免影响后续面板和视口绘制。
        renderer.SetClipRect(window, {0.0f, 0.0f}, {0.0f, 0.0f});
    }
}
