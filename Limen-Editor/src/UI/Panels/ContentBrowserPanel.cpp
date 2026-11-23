#include "UI/Panels/ContentBrowserPanel.h"

#include <algorithm>

#include "Limen/Application/Window.h"
#include "Limen/Input/Input.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Project/EditorSession.h"
#include "Render/EditorUIRenderer.h"
#include "UI/EditorTheme.h"

namespace LimenEditor
{
    void ContentBrowserPanel::Update(const EditorSession &session)
    {
        Limen::LArray<Limen::AssetRecord> rows;
        session.GetAssetRegistry().ForEachRecord(
            [&](const Limen::AssetRecord &record)
            {
                // 只有存活记录会进入视图；后续可在这里过滤资产类型。
                rows.push_back(record);
            });
        m_Rows = rows;
    }

    void ContentBrowserPanel::Draw(
        const EditorSession &session,
        const Limen::Window &window,
        const EditorUIRenderer &renderer,
        const Limen::FontAtlas &fontAtlas,
        const EditorTheme &theme,
        const UIRect &rect)
    {
        m_Rect = rect;
        m_PlaceButtonRect = {};
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
            "CONTENT BROWSER");

        const float placeButtonWidth =
            std::min(58.0f, rect.Width - 16.0f);
        if (placeButtonWidth > 0.0f)
        {
            const float buttonHeight =
                std::min(22.0f, headerHeight - 6.0f);
            m_PlaceButtonRect = {
                rect.X + rect.Width - placeButtonWidth - 8.0f,
                rect.Y + (headerHeight - buttonHeight) * 0.5f,
                placeButtonWidth,
                buttonHeight
            };

            const auto [mouseX, mouseY] = Limen::Input::GetMousePos();
            const bool canPlace = session.GetSelectedAssetID().isValid();
            const bool hovered = m_PlaceButtonRect.Contains(mouseX, mouseY);
            renderer.DrawSolidRect(
                window,
                {m_PlaceButtonRect.X, m_PlaceButtonRect.Y},
                {m_PlaceButtonRect.Width, m_PlaceButtonRect.Height},
                canPlace ? theme.ButtonBorder : theme.PanelBorder);
            renderer.DrawSolidRect(
                window,
                {m_PlaceButtonRect.X + 1.0f, m_PlaceButtonRect.Y + 1.0f},
                {m_PlaceButtonRect.Width - 2.0f,
                 m_PlaceButtonRect.Height - 2.0f},
                canPlace
                    ? (hovered
                           ? theme.ButtonHoverBackground
                           : theme.ButtonBackground)
                    : theme.PanelBackground);
            renderer.DrawText(
                window,
                {m_PlaceButtonRect.X + 8.0f, m_PlaceButtonRect.Y + 1.0f},
                canPlace ? theme.ButtonText : theme.TextMuted,
                fontAtlas,
                "PLACE");
        }

        m_FirstRowY = rect.Y + headerHeight + 6.0f;
        m_RowHeight = rowHeight;

        if (m_Rows.getSize() == 0)
        {
            renderer.DrawText(
                window,
                {rect.X + theme.PanelPadding, m_FirstRowY},
                theme.TextMuted,
                fontAtlas,
                "No assets");
        }

        for (uint32_t row = 0; row < m_Rows.getSize(); ++row)
        {
            const Limen::AssetRecord &record = m_Rows[row];
            const float rowY = m_FirstRowY + rowHeight * static_cast<float>(row);
            const bool selected = session.IsAssetSelected(record.ID);

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

            const float iconSize = 12.0f;
            const float iconX = rect.X + theme.PanelPadding;
            const float iconY = rowY + 3.0f;
            renderer.DrawSolidRect(
                window,
                {iconX, iconY},
                {iconSize, iconSize},
                selected
                    ? theme.SelectionAccent
                    : theme.ButtonBorder);
            renderer.DrawSolidRect(
                window,
                {iconX + 2.0f, iconY + 2.0f},
                {iconSize - 4.0f, iconSize - 4.0f},
                selected
                    ? theme.SelectionBackground
                    : theme.ButtonBackground);

            glm::vec4 color = selected ? theme.SelectionText : theme.TextSecondary;
            if (!selected && record.Status == Limen::AssetRecordStatus::Missing)
                color = theme.MissingText;

            renderer.DrawText(
                window,
                {iconX + iconSize + 7.0f, rowY},
                color,
                fontAtlas,
                std::string_view(
                    record.LogicalPath.data(),
                    record.LogicalPath.size()));
        }

        renderer.SetClipRect(window, {0.0f, 0.0f}, {0.0f, 0.0f});
    }

    ContentBrowserAction ContentBrowserPanel::OnMousePressed(
        EditorSession &session,
        const float x,
        const float y)
    {
        if (!m_Rect.Contains(x, y) || m_RowHeight <= 0.0f)
        {
            m_ClickState.Reset();
            m_DragState.Cancel();
            return ContentBrowserAction::None;
        }

        if (m_PlaceButtonRect.Contains(x, y))
        {
            m_ClickState.Reset();
            m_DragState.Cancel();
            if (!session.GetSelectedAssetID().isValid())
                return ContentBrowserAction::None;

            return m_PlaceCommandGuard.TryAcquire(
                       std::chrono::steady_clock::now())
                       ? ContentBrowserAction::PlaceAsset
                       : ContentBrowserAction::None;
        }

        const int row = FindListRowAt(
            m_FirstRowY,
            m_RowHeight,
            y,
            m_Rows.getSize());
        if (row < 0)
        {
            m_ClickState.Reset();
            m_DragState.Cancel();
            return ContentBrowserAction::None;
        }

        const Limen::AssetID id =
            m_Rows[static_cast<uint32_t>(row)].ID;
        if (!session.SelectAsset(id))
        {
            m_ClickState.Reset();
            m_DragState.Cancel();
            return ContentBrowserAction::None;
        }

        const bool activate =
            m_ClickState.RegisterPress(id, std::chrono::steady_clock::now()) ==
            ContentBrowserClickState::Result::ActivateAsset;
        m_DragState.Begin(id, x, y);

        return activate
                   ? ContentBrowserAction::ActivateAsset
                   : ContentBrowserAction::SelectionChanged;
    }

    void ContentBrowserPanel::OnMouseMoved(
        const float x,
        const float y) noexcept
    {
        m_DragState.Update(x, y);
    }

    bool ContentBrowserPanel::EndDrag() noexcept
    {
        return m_DragState.End();
    }

    bool ContentBrowserPanel::IsDraggingAsset() const noexcept
    {
        return m_DragState.IsDragging();
    }
}
