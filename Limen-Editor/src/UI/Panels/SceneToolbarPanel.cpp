#include "UI/Panels/SceneToolbarPanel.h"

#include "Limen/Input/Input.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Render/EditorUIRenderer.h"
#include "UI/EditorTheme.h"

namespace LimenEditor
{
    void SceneToolbarPanel::Draw(
        const Limen::Window &window,
        const EditorUIRenderer &renderer,
        const Limen::FontAtlas &fontAtlas,
        const EditorTheme &theme,
        const UIRect &rect)
    {
        Reset();

        constexpr float labelWidth = 44.0f;
        constexpr float buttonGap = 6.0f;

        float totalWidth = labelWidth;
        for (const auto &button : m_Buttons)
            totalWidth += buttonGap + button.Width;

        if (rect.Height <= 0.0f || totalWidth > rect.Width)
            return;

        renderer.SetClipRect(window, {rect.X, rect.Y}, {rect.Width, rect.Height});
        const float textY = rect.Y + (rect.Height - fontAtlas.GetLineHeight()) * 0.5f;
        renderer.DrawText(window, {rect.X, textY}, theme.TextMuted, fontAtlas, "File");

        const auto [mouseX, mouseY] = Limen::Input::GetMousePos();
        float buttonX = rect.X + labelWidth + buttonGap;
        for (auto &button : m_Buttons)
        {
            button.Rect = {buttonX, rect.Y, button.Width, rect.Height};
            renderer.DrawRoundedRect(
                window,
                {buttonX, rect.Y},
                {button.Width, rect.Height},
                button.Rect.Contains(mouseX, mouseY) ? theme.ButtonHoverBackground : theme.ButtonBackground,
                6.0f);
            const float textWidth = renderer.MeasureTextWidth(fontAtlas, button.Label);
            renderer.DrawText(window, {buttonX + (button.Width - textWidth) * 0.5f, textY},
                              theme.ButtonText, fontAtlas, button.Label);
            buttonX += button.Width + buttonGap;
        }

        renderer.SetClipRect(window, {0.0f, 0.0f}, {0.0f, 0.0f});
    }

    SceneToolbarAction SceneToolbarPanel::HitTest(const float x, const float y) const noexcept
    {
        for (const auto &button : m_Buttons)
            if (button.Rect.Contains(x, y))
                return button.Action;

        return SceneToolbarAction::None;
    }

    void SceneToolbarPanel::Reset() noexcept
    {
        for (auto &button : m_Buttons)
            button.Rect = {};
    }
}
