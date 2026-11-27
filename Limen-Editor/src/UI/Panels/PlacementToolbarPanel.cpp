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
            renderer.DrawRoundedRect(
                window,
                {rect.X, rect.Y},
                {rect.Width, rect.Height},
                hovered ? theme.ButtonHoverBackground : theme.ButtonBackground,
                6.0f);
            const float textWidth = renderer.MeasureTextWidth(fontAtlas, label);
            renderer.DrawText(
                window,
                {rect.X + (rect.Width - textWidth) * 0.5f,
                 rect.Y + (rect.Height - fontAtlas.GetLineHeight()) * 0.5f},
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
        const float rowHeight = rect.Height;
        UpdateLayout(rect, rowHeight);
        if (m_Layout.CubeButton.Width <= 0.0f)
            return;

        renderer.SetClipRect(
            window,
            {rect.X, rect.Y},
            {rect.Width, rect.Height});

        if (m_Layout.Label.Width > 0.0f)
        {
            renderer.DrawText(
                window,
                {m_Layout.Label.X, m_Layout.Label.Y + (rowHeight - fontAtlas.GetLineHeight()) * 0.5f},
                theme.TextMuted,
                fontAtlas,
                "Add");
        }
        if (m_Layout.CubeButton.Width > 0.0f)
        {
            DrawButton(
                window,
                renderer,
                fontAtlas,
                theme,
                m_Layout.CubeButton,
                "Cube");
        }
        if (m_Layout.PointLightButton.Width > 0.0f)
        {
            DrawButton(
                window,
                renderer,
                fontAtlas,
                theme,
                m_Layout.PointLightButton,
                "Point Light");
        }

        renderer.SetClipRect(window, {0.0f, 0.0f}, {0.0f, 0.0f});
    }
}
