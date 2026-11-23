#include "UI/Panels/PlacementToolbarPanel.h"

#include "Limen/Application/Window.h"
#include "Limen/Input/Input.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Render/EditorUIRenderer.h"
#include "UI/EditorTheme.h"

namespace LimenEditor
{
    namespace
    {
        void DrawButton(
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &fontAtlas,
            const EditorTheme &theme,
            const UIRect &rect,
            const char *label)
        {
            const auto [mouseX, mouseY] = Limen::Input::GetMousePos();
            const bool hovered = rect.Contains(mouseX, mouseY);
            renderer.DrawSolidRect(
                window,
                {rect.X, rect.Y},
                {rect.Width, rect.Height},
                theme.ButtonBorder);
            renderer.DrawSolidRect(
                window,
                {rect.X + 1.0f, rect.Y + 1.0f},
                {rect.Width - 2.0f, rect.Height - 2.0f},
                hovered
                    ? theme.ButtonHoverBackground
                    : theme.ButtonBackground);
            renderer.DrawText(
                window,
                {rect.X + 9.0f, rect.Y + 1.0f},
                theme.ButtonText,
                fontAtlas,
                label);
        }
    }

    void PlacementToolbarPanel::Draw(
        const Limen::Window &window,
        const EditorUIRenderer &renderer,
        const Limen::FontAtlas &fontAtlas,
        const EditorTheme &theme,
        const UIRect &rect)
    {
        const float rowHeight =
            fontAtlas.GetLineHeight() + theme.RowSpacing;
        UpdateLayout(rect, rowHeight);

        renderer.SetClipRect(
            window,
            {rect.X, rect.Y},
            {rect.Width, rect.Height});

        if (m_Layout.Label.Width > 0.0f)
        {
            renderer.DrawText(
                window,
                {m_Layout.Label.X, m_Layout.Label.Y},
                theme.TextSecondary,
                fontAtlas,
                "PLACE");
        }
        if (m_Layout.CubeButton.Width > 0.0f)
        {
            DrawButton(
                window,
                renderer,
                fontAtlas,
                theme,
                m_Layout.CubeButton,
                "CUBE");
        }
        if (m_Layout.PointLightButton.Width > 0.0f)
        {
            DrawButton(
                window,
                renderer,
                fontAtlas,
                theme,
                m_Layout.PointLightButton,
                "POINT LIGHT");
        }

        renderer.SetClipRect(window, {0.0f, 0.0f}, {0.0f, 0.0f});
    }
}
