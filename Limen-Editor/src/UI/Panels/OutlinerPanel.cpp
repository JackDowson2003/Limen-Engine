#include "UI/Panels/OutlinerPanel.h"

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

        const float rowHeight = fontAtlas.GetLineHeight() + theme.RowSpacing;
        const float textX = rect.X + theme.PanelPadding;
        const float headerY = rect.Y + theme.HeaderOffset;

        renderer.SetClipRect(
            window,
            {rect.X, rect.Y},
            {rect.Width, rect.Height});

        renderer.DrawText(
            window,
            {textX, headerY},
            theme.TextPrimary,
            fontAtlas,
            "Outliner");

        m_FirstRowY = headerY + rowHeight;
        m_RowHeight = rowHeight;

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
                    {rect.X + 4.0f, rowY - 2.0f},
                    {rect.Width - 8.0f, rowHeight},
                    theme.SelectionBackground);
            }

            renderer.DrawText(
                window,
                {textX, rowY},
                selected ? theme.SelectionText : theme.TextSecondary,
                fontAtlas,
                "对象 " + std::to_string(row + 1));
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
