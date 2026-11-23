#include "UI/Panels/OutlinerPanel.h"

#include <algorithm>
#include <string>

#include "Limen/Application/Window.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Project/EditorSession.h"
#include "Render/EditorUIRenderer.h"
#include "UI/EditorTheme.h"

namespace LimenEditor
{
    void OutlinerPanel::Update(const EditorSession &session)
    {
        Limen::LArray<Limen::SceneObjectID> rows;
        session.GetScene().ForEachObject(
            [&](const Limen::SceneObjectHandle &,
                const Limen::SceneObjectID &id)
            {
                rows.push_back(id);
            });
        m_Rows = rows;
    }

    void OutlinerPanel::Draw(
        const EditorSession &session,
        const Limen::Window &window,
        const EditorUIRenderer &renderer,
        const Limen::FontAtlas &fontAtlas,
        const EditorTheme &theme,
        const UIRect &rect)
    {
        m_Rect = rect;

        const float lineHeight = fontAtlas.GetLineHeight();
        const float rowHeight = lineHeight + theme.RowSpacing;
        const float headerHeight = theme.PanelHeaderHeight;
        const float headerY =
            rect.Y + std::max(0.0f, (headerHeight - lineHeight) * 0.5f);

        renderer.SetClipRect(
            window,
            {rect.X, rect.Y},
            {rect.Width, rect.Height});

        renderer.DrawSolidRect(
            window,
            {rect.X, rect.Y},
            {rect.Width, headerHeight},
            theme.PanelHeaderBackground);
        renderer.DrawSolidRect(
            window,
            {rect.X, rect.Y + headerHeight - 1.0f},
            {rect.Width, 1.0f},
            theme.PanelBorder);
        renderer.DrawSolidRect(
            window,
            {rect.X + 8.0f, headerY},
            {2.0f, lineHeight},
            theme.Accent);
        renderer.DrawText(
            window,
            {rect.X + 16.0f, headerY},
            theme.TextPrimary,
            fontAtlas,
            "OUTLINER");
        const std::string countText = std::to_string(m_Rows.getSize());
        renderer.DrawText(
            window,
            {rect.X + rect.Width - theme.PanelPadding -
                 static_cast<float>(countText.size()) * 8.0f,
             headerY},
            theme.TextMuted,
            fontAtlas,
            countText);

        m_FirstRowY = rect.Y + headerHeight + 6.0f;
        m_RowHeight = rowHeight;

        if (m_Rows.getSize() == 0)
        {
            renderer.DrawText(
                window,
                {rect.X + theme.PanelPadding, m_FirstRowY},
                theme.TextMuted,
                fontAtlas,
                "No actors");
        }

        for (uint32_t row = 0; row < m_Rows.getSize(); ++row)
        {
            const Limen::SceneObjectID &id = m_Rows[row];
            const bool selected = session.IsObjectSelected(id);
            const float rowY =
                m_FirstRowY + rowHeight * static_cast<float>(row);

            // UE 风格：选中行整行蓝色背景，文字保持白色。
            if (selected)
            {
                renderer.DrawSolidRect(
                    window,
                    {rect.X + 4.0f, rowY - 1.0f},
                    {rect.Width - 8.0f, rowHeight},
                    theme.SelectionBackground);
                renderer.DrawSolidRect(
                    window,
                    {rect.X + 4.0f, rowY - 1.0f},
                    {2.0f, rowHeight},
                    theme.SelectionAccent);
            }

            Limen::LString label;
            const bool hasLabel =
                session.TryGetObjectLabel(id, label) && !label.empty();
            const std::string displayName = hasLabel
                ? std::string(label.data(), label.size())
                : "Object " + std::to_string(row + 1);
            const float iconX = rect.X + theme.PanelPadding;
            const float iconY = rowY + 3.0f;
            renderer.DrawSolidRect(
                window,
                {iconX, iconY},
                {12.0f, 12.0f},
                selected
                    ? theme.SelectionAccent
                    : theme.ButtonBorder);
            renderer.DrawText(
                window,
                {iconX + 3.0f, iconY - 2.0f},
                selected
                    ? theme.SelectionText
                    : theme.TextMuted,
                fontAtlas,
                ">");
            renderer.DrawText(
                window,
                {iconX + 19.0f, rowY},
                selected ? theme.SelectionText : theme.TextSecondary,
                fontAtlas,
                displayName);
        }

        renderer.SetClipRect(window, {0.0f, 0.0f}, {0.0f, 0.0f});
    }

    OutlinerPanelAction OutlinerPanel::OnMousePressed(
        EditorSession &session,
        const float x,
        const float y,
        const bool toggle,
        const bool range)
    {
        if (!m_Rect.Contains(x, y) || m_RowHeight <= 0.0f)
            return OutlinerPanelAction::None;

        const int row = FindListRowAt(
            m_FirstRowY,
            m_RowHeight,
            y,
            m_Rows.getSize());
        if (row < 0)
            return OutlinerPanelAction::None;

        const Limen::SceneObjectID clicked =
            m_Rows[static_cast<uint32_t>(row)];

        if (range && m_SelectionAnchor.isValid())
        {
            const auto selection = BuildSelectionRange(
                m_Rows,
                m_SelectionAnchor,
                clicked);
            session.SelectObjects(selection);
        }
        else if (toggle)
        {
            session.ToggleObjectSelection(clicked);
            if (session.IsObjectSelected(clicked))
                m_SelectionAnchor = clicked;
        }
        else
        {
            session.SelectObject(clicked);
            m_SelectionAnchor = clicked;
        }

        return OutlinerPanelAction::SelectionChanged;
    }

    void OutlinerPanel::SetSelectionAnchor(
        const Limen::SceneObjectID id) noexcept
    {
        m_SelectionAnchor = id;
    }
}
