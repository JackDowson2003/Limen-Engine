#include "UI/Panels/PlacementToolbarPanel.h"

#include "Limen/Application/Window.h"
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
            renderer.DrawSolidRect(
                window,
                {rect.X, rect.Y},
                {rect.Width, rect.Height},
                theme.ButtonBackground);
            renderer.DrawText(
                window,
                {rect.X + 8.0f, rect.Y + 2.0f},
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
                "Place");
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
