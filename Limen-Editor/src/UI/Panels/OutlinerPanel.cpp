#include "UI/Panels/OutlinerPanel.h"

#include <algorithm>
#include <string>
#include <string_view>

#include "Limen/Application/Window.h"
#include "Limen/Input/Input.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Project/EditorSession.h"
#include "Render/EditorUIRenderer.h"
#include "UI/EditorTheme.h"
#include "UI/Panels/SidePanelDrawing.h"

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
        m_RowHeight = 0.0f;
        if (rect.Width <= 0.0f || rect.Height <= 0.0f)
            return;

        const float lineHeight = fontAtlas.GetLineHeight();
        const float rowHeight = lineHeight + theme.RowSpacing + 6.0f;
        const float headerHeight = theme.PanelHeaderHeight;
        const float padding = theme.PanelPadding;
        const float rightEdge = rect.X + rect.Width - padding;
        const float contentWidth = rect.Width - padding * 2.0f;
        const auto [mouseX, mouseY] = Limen::Input::GetMousePos();

        renderer.SetClipRect(
            window,
            {rect.X, rect.Y},
            {rect.Width, rect.Height});

        constexpr std::string_view title = "Outliner";
        const std::string countText = std::to_string(m_Rows.getSize());
        const float countWidth = renderer.MeasureTextWidth(fontAtlas, countText);
        const bool showCount = m_Rows.getSize() != 0 &&
                               contentWidth >= renderer.MeasureTextWidth(fontAtlas, title) + countWidth + 12.0f;
        SidePanelDrawing::DrawHeader(window, renderer, fontAtlas, theme, rect, title,
                                     showCount ? countWidth + 12.0f : 0.0f);
        if (showCount)
            renderer.DrawText(window,
                              {rightEdge - countWidth, rect.Y + std::max(0.0f, (headerHeight - lineHeight) * 0.5f)},
                              theme.TextMuted, fontAtlas, countText);

        m_FirstRowY = rect.Y + headerHeight + 8.0f;
        m_RowHeight = rowHeight;

        if (m_Rows.getSize() == 0)
            SidePanelDrawing::DrawText(window, renderer, fontAtlas, {rect.X + padding, m_FirstRowY},
                                       theme.TextMuted, contentWidth, "No scene objects");

        for (uint32_t row = 0; row < m_Rows.getSize(); ++row)
        {
            const float rowY = m_FirstRowY + rowHeight * static_cast<float>(row);
            if (rowY >= rect.Y + rect.Height)
                break;

            const Limen::SceneObjectID &id = m_Rows[row];
            const bool selected = session.IsObjectSelected(id);
            const bool hovered = rect.Contains(mouseX, mouseY) && mouseY >= rowY && mouseY < rowY + rowHeight;
            if ((selected || hovered) && rect.Width > padding)
                renderer.DrawRoundedRect(window, {rect.X + padding * 0.5f, rowY + 1.0f},
                                         {rect.Width - padding, rowHeight - 2.0f},
                                         selected ? theme.SelectionBackground : theme.RowHoverBackground, 6.0f);

            const float iconX = rect.X + padding;
            const float iconY = rowY + (rowHeight - 12.0f) * 0.5f;
            if (contentWidth >= 12.0f)
            {
                // 简洁的节点符号区分场景对象和左栏中的资产文件。
                const auto iconColor = selected ? theme.SelectionAccent : theme.TextMuted;
                renderer.DrawRoundedRect(window, {iconX + 3.0f, iconY}, {6.0f, 6.0f}, iconColor, 2.0f);
                renderer.DrawSolidRect(window, {iconX + 5.5f, iconY + 5.0f}, {1.0f, 5.0f}, iconColor);
                renderer.DrawRoundedRect(window, {iconX, iconY + 9.0f}, {12.0f, 3.0f}, iconColor, 1.5f);
            }

            Limen::LString label;
            const bool hasLabel =
                session.TryGetObjectLabel(id, label) && !label.empty();
            const std::string displayName = hasLabel
                ? std::string(label.data(), label.size())
                : "Object " + std::to_string(row + 1);
            const float textX = iconX + 22.0f;
            SidePanelDrawing::DrawText(window, renderer, fontAtlas, {textX, rowY + (rowHeight - lineHeight) * 0.5f},
                                       selected ? theme.SelectionText : theme.TextSecondary,
                                       rightEdge - textX, displayName);
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
