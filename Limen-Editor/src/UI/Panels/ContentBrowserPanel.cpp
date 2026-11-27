#include "UI/Panels/ContentBrowserPanel.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>

#include "Limen/Application/Window.h"
#include "Limen/Input/Input.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Project/EditorSession.h"
#include "Render/EditorUIRenderer.h"
#include "UI/EditorTheme.h"
#include "UI/Panels/SidePanelDrawing.h"

namespace LimenEditor
{
    namespace
    {
        [[nodiscard]] std::string_view GetAssetPath(const Limen::AssetRecord &asset)
        {
            return {asset.LogicalPath.data(), asset.LogicalPath.size()};
        }

        [[nodiscard]] Limen::LArray<std::string> ReadChildFolders(
            const std::filesystem::path &assetRoot, const std::string_view logicalPath)
        {
            Limen::LArray<std::string> folders;
            // 逐级检查目录类型，避免浏览窗口通过符号链接走出项目资产根目录。
            std::error_code error;
            if (!std::filesystem::is_directory(std::filesystem::symlink_status(assetRoot, error)))
                return folders;
            auto directory = assetRoot;
            const std::filesystem::path relativeDirectory{std::u8string(logicalPath.begin(), logicalPath.end())};
            for (const auto &component : relativeDirectory)
            {
                directory /= component;
                if (!std::filesystem::is_directory(std::filesystem::symlink_status(directory, error)))
                    return folders;
            }

            for (std::filesystem::directory_iterator entry(directory, error), end;
                 !error && entry != end; entry.increment(error))
            {
                std::error_code typeError;
                if (!std::filesystem::is_directory(entry->symlink_status(typeError)))
                    continue;
                const auto name = entry->path().filename().u8string();
                if (!name.empty() && name.front() != u8'.')
                    folders.emplace_back(name.begin(), name.end());
            }
            if (folders.getSize() > 1)
                std::sort(folders.begin(), folders.end());
            return folders;
        }

        [[nodiscard]] bool CanPlaceAsset(const Limen::AssetRecord &asset)
        {
            return asset.Status == Limen::AssetRecordStatus::Present &&
                   DescribeContentAssetPath(GetAssetPath(asset)).Kind == ContentAssetKind::Mesh;
        }

        [[nodiscard]] std::string_view DescribeAssetKind(const ContentAssetKind kind)
        {
            switch (kind)
            {
                case ContentAssetKind::Mesh: return "OBJ Mesh";
                case ContentAssetKind::Scene: return "Scene";
                case ContentAssetKind::Image: return "Image";
                case ContentAssetKind::Material: return "MTL Material";
                case ContentAssetKind::Folder: return "Folder";
                case ContentAssetKind::File: return "File";
            }
            return "File";
        }

        void DrawAssetGlyph(
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const glm::vec2 origin,
            const glm::vec4 &color,
            const glm::vec4 &background,
            const ContentAssetKind kind)
        {
            const auto rect = [&](const glm::vec2 offset, const glm::vec2 size, const glm::vec4 &tint)
            {
                renderer.DrawRoundedRect(window, origin + offset, size, tint, 2.0f);
            };
            switch (kind)
            {
                case ContentAssetKind::Mesh:
                    rect({7.0f, 0.0f}, {17.0f, 17.0f}, color);
                    rect({4.0f, 3.0f}, {17.0f, 17.0f}, background);
                    rect({0.0f, 7.0f}, {17.0f, 17.0f}, color);
                    rect({4.0f, 11.0f}, {9.0f, 9.0f}, background);
                    break;
                case ContentAssetKind::Scene:
                    rect({0.0f, 3.0f}, {24.0f, 19.0f}, color);
                    rect({3.0f, 7.0f}, {18.0f, 12.0f}, background);
                    rect({5.0f, 12.0f}, {4.0f, 5.0f}, color);
                    rect({10.0f, 9.0f}, {4.0f, 8.0f}, color);
                    rect({15.0f, 13.0f}, {4.0f, 4.0f}, color);
                    break;
                case ContentAssetKind::Image:
                    rect({0.0f, 3.0f}, {24.0f, 19.0f}, color);
                    rect({3.0f, 6.0f}, {18.0f, 13.0f}, background);
                    rect({6.0f, 8.0f}, {4.0f, 4.0f}, color);
                    rect({12.0f, 13.0f}, {8.0f, 4.0f}, color);
                    break;
                case ContentAssetKind::Material:
                    renderer.DrawRoundedRect(window, origin + glm::vec2{2.0f, 2.0f},
                                             {20.0f, 20.0f}, color, 10.0f);
                    rect({7.0f, 6.0f}, {5.0f, 5.0f}, background);
                    break;
                case ContentAssetKind::Folder:
                    rect({0.0f, 4.0f}, {11.0f, 10.0f}, color);
                    rect({0.0f, 8.0f}, {24.0f, 14.0f}, color);
                    break;
                case ContentAssetKind::File:
                    rect({3.0f, 0.0f}, {18.0f, 24.0f}, color);
                    rect({7.0f, 8.0f}, {10.0f, 2.0f}, background);
                    rect({7.0f, 14.0f}, {7.0f, 2.0f}, background);
                    break;
            }
        }
    }

    bool ContentBrowserPanel::Update(const EditorSession &session)
    {
        const auto &assetRoot = session.GetConfiguration().AssetRootAbsolute;
        if (m_AssetRoot != assetRoot)
        {
            m_AssetRoot = assetRoot;
            ChangeFolder({});
        }

        bool changed = m_FolderChanged;
        const auto now = std::chrono::steady_clock::now();
        if (m_FolderChanged || now >= m_NextDirectoryRefresh)
        {
            // 当前目录允许显示空目录。限制磁盘枚举频率，避免每帧访问磁盘。
            auto folders = ReadChildFolders(m_AssetRoot, m_CurrentFolder);
            if (m_DiskFolders.getSize() != folders.getSize() ||
                !std::equal(m_DiskFolders.begin(), m_DiskFolders.end(), folders.begin()))
            {
                m_DiskFolders = std::move(folders);
                changed = true;
            }
            m_NextDirectoryRefresh = now + std::chrono::seconds(1);
            m_FolderChanged = false;
        }

        std::uint32_t index = 0;
        session.GetAssetRegistry().ForEachRecord(
            [&](const Limen::AssetRecord &record)
            {
                if (index >= m_Assets.getSize())
                {
                    m_Assets.push_back(record);
                    changed = true;
                }
                else
                {
                    auto &cached = m_Assets[index];
                    if (cached.ID != record.ID || cached.LogicalPath != record.LogicalPath ||
                        cached.Status != record.Status)
                    {
                        cached = record;
                        changed = true;
                    }
                }
                ++index;
            });
        while (m_Assets.getSize() > index)
        {
            m_Assets.pop_back();
            changed = true;
        }
        if (!changed)
            return false;

        // 保留 Registry 原顺序检测变化。显示当前目录的直接资源，已丢失资源所在目录也保留入口。
        m_ChildFolders = m_DiskFolders;
        m_AssetOrder.clear();
        for (std::uint32_t i = 0; i < m_Assets.getSize(); ++i)
        {
            const auto relative = RelativeContentPath(GetAssetPath(m_Assets[i]), m_CurrentFolder);
            if (relative.empty())
                continue;
            const auto slash = relative.find('/');
            if (slash == std::string_view::npos)
                m_AssetOrder.push_back(i);
            else
                m_ChildFolders.emplace_back(relative.substr(0, slash));
        }
        if (m_ChildFolders.getSize() > 1)
        {
            std::sort(m_ChildFolders.begin(), m_ChildFolders.end());
            const auto uniqueEnd = std::unique(m_ChildFolders.begin(), m_ChildFolders.end());
            while (m_ChildFolders.end() != uniqueEnd)
                m_ChildFolders.pop_back();
        }
        if (m_AssetOrder.getSize() > 1)
        {
            std::sort(m_AssetOrder.begin(), m_AssetOrder.end(),
                [&](const std::uint32_t left, const std::uint32_t right)
                {
                    return m_Assets[left].LogicalPath < m_Assets[right].LogicalPath;
                });
        }
        return true;
    }

    void ContentBrowserPanel::AppendVisibleLabels(std::string &text) const
    {
        // 加入空格分隔各个 UTF-8 文本，供绘制这些标签前预加载字体图集。
        text += m_CurrentFolder;
        for (const auto &folder : m_ChildFolders)
        {
            text += ' ';
            text += folder;
        }
        for (const auto index : m_AssetOrder)
        {
            text += ' ';
            text += GetAssetPath(m_Assets[index]);
        }
    }

    void ContentBrowserPanel::ChangeFolder(std::string logicalPath)
    {
        m_CurrentFolder = std::move(logicalPath);
        m_Scroll = 0.0f;
        m_FolderChanged = true;
        // 等下一次绘制更新命中区域，避免同一轮事件落到旧目录的卡片上。
        ResetLayout();
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
        m_ParentButtonRect = {};
        m_PlaceButtonRect = {};
        m_Grid = {};
        if (rect.Width <= 0.0f || rect.Height <= 0.0f)
        {
            ResetLayout();
            return;
        }

        const float lineHeight = fontAtlas.GetLineHeight();
        const float headerHeight = theme.PanelHeaderHeight;
        const float padding = theme.PanelPadding;
        const float left = rect.X + padding;
        const float right = rect.X + rect.Width - padding;
        const float contentWidth = right - left;
        const auto [mouseX, mouseY] = Limen::Input::GetMousePos();
        renderer.SetClipRect(window, {rect.X, rect.Y}, {rect.Width, rect.Height});

        std::string title = "Assets";
        for (auto component = std::string_view{m_CurrentFolder}; !component.empty();)
        {
            const auto slash = component.find('/');
            title += " / ";
            title += component.substr(0, slash);
            if (slash == std::string_view::npos)
                break;
            component.remove_prefix(slash + 1);
        }
        constexpr std::string_view parentLabel = "Up";
        constexpr std::string_view placeLabel = "Place";
        const float placeTextWidth = renderer.MeasureTextWidth(fontAtlas, placeLabel);
        const float placeButtonWidth = placeTextWidth + 24.0f;
        const float buttonHeight = lineHeight + 6.0f;
        constexpr float headerGap = 12.0f;
        const float buttonY = rect.Y + (headerHeight - buttonHeight) * 0.5f;
        const float headerTextY = rect.Y + (headerHeight - lineHeight) * 0.5f;
        const bool buttonsFitVertically = headerHeight >= buttonHeight && rect.Height >= headerHeight;
        float titleX = left;
        const float parentButtonWidth = renderer.MeasureTextWidth(fontAtlas, parentLabel) + 20.0f;
        if (!m_CurrentFolder.empty() && buttonsFitVertically && contentWidth >= parentButtonWidth)
        {
            m_ParentButtonRect = {left, buttonY, parentButtonWidth, buttonHeight};
            const bool hovered = m_ParentButtonRect.Contains(mouseX, mouseY);
            renderer.DrawRoundedRect(window, {left, buttonY}, {parentButtonWidth, buttonHeight},
                                     hovered ? theme.ButtonHoverBackground : theme.ButtonBackground, 6.0f);
            renderer.DrawText(window, {left + 10.0f, headerTextY}, theme.ButtonText, fontAtlas, parentLabel);
            titleX += parentButtonWidth + headerGap;
        }

        Limen::AssetRecord selectedAsset;
        const bool hasAsset = session.GetAssetRegistry().TryFind(session.GetSelectedAssetID(), selectedAsset);
        const bool canPlace = hasAsset && CanPlaceAsset(selectedAsset);
        const float minTitleWidth = renderer.MeasureTextWidth(fontAtlas, "Assets");
        const bool showPlace = buttonsFitVertically && right - titleX >= minTitleWidth + placeButtonWidth + headerGap;
        const float headerRight = right - (showPlace ? placeButtonWidth + headerGap : 0.0f);
        SidePanelDrawing::DrawText(window, renderer, fontAtlas, {titleX, headerTextY}, theme.TextPrimary,
                                   headerRight - titleX, title);
        if (contentWidth > 0.0f && rect.Height >= headerHeight)
            renderer.DrawSolidRect(window, {left, rect.Y + headerHeight - 1.0f},
                                   {contentWidth, 1.0f}, theme.PanelBorder);
        if (showPlace)
        {
            m_PlaceButtonRect = {right - placeButtonWidth, buttonY, placeButtonWidth, buttonHeight};
            const bool hovered = m_PlaceButtonRect.Contains(mouseX, mouseY);
            if (canPlace)
                renderer.DrawRoundedRect(
                    window, {m_PlaceButtonRect.X, m_PlaceButtonRect.Y},
                    {placeButtonWidth, buttonHeight},
                    hovered ? theme.ButtonHoverBackground : theme.ButtonBackground, 6.0f);
            renderer.DrawText(window, {m_PlaceButtonRect.X + 12.0f, headerTextY},
                              canPlace ? theme.ButtonText : theme.TextMuted, fontAtlas, placeLabel);
        }
        if (hasAsset)
        {
            const float pathX = titleX + renderer.MeasureTextWidth(fontAtlas, title) + headerGap * 2.0f;
            const auto path = GetAssetPath(selectedAsset);
            const auto color = selectedAsset.Status == Limen::AssetRecordStatus::Missing
                ? theme.MissingText : theme.TextSecondary;
            SidePanelDrawing::DrawText(window, renderer, fontAtlas, {pathX, headerTextY}, color,
                                       headerRight - pathX, path);
        }

        constexpr float gridPadding = 7.0f;
        constexpr float scrollbarReserve = 7.0f;
        const UIRect gridRect{
            left, rect.Y + headerHeight + gridPadding, contentWidth - scrollbarReserve,
            rect.Height - headerHeight - gridPadding * 2.0f
        };
        const auto folderCount = m_ChildFolders.getSize();
        const auto entryCount = folderCount + m_AssetOrder.getSize();
        m_Grid = ContentBrowserGrid::Calculate(gridRect, entryCount);
        const float scrollLimit = m_Grid.ScrollLimit();
        m_Scroll = std::clamp(m_Scroll, 0.0f, scrollLimit);
        m_PaintedScroll = m_Scroll;

        if (m_Grid.Columns != 0)
        {
            // 滚动裁剪只覆盖网格。部分露出的资源卡片不能盖住标题，也不能在标题中命中。
            renderer.SetClipRect(window, {gridRect.X, gridRect.Y}, {gridRect.Width, gridRect.Height});
            if (entryCount == 0)
            {
                SidePanelDrawing::DrawText(window, renderer, fontAtlas, {left, gridRect.Y + 8.0f},
                                           theme.TextMuted, gridRect.Width, "No folders or registered assets");
            }
            const float bottom = gridRect.Y + gridRect.Height;
            const auto firstRow = static_cast<std::uint32_t>(m_PaintedScroll / ContentBrowserGrid::RowPitch);
            for (std::uint32_t index = firstRow * m_Grid.Columns; index < entryCount; ++index)
            {
                const UIRect tile = m_Grid.TileAt(index, m_PaintedScroll);
                if (tile.Y >= bottom)
                    break;

                const auto *asset = index < folderCount ? nullptr : &m_Assets[m_AssetOrder[index - folderCount]];
                const bool selected = asset && session.IsAssetSelected(asset->ID);
                const bool missing = asset && asset->Status == Limen::AssetRecordStatus::Missing;
                const bool hovered = gridRect.Contains(mouseX, mouseY) && tile.Contains(mouseX, mouseY);
                const auto tileColor = selected ? theme.SelectionBackground :
                    (hovered ? theme.RowHoverBackground : theme.TopBarBackground);
                renderer.DrawRoundedRect(window, {tile.X, tile.Y}, {tile.Width, tile.Height}, tileColor, 7.0f);

                const auto label = asset ? DescribeContentAssetPath(GetAssetPath(*asset)) :
                    ContentAssetLabel{m_ChildFolders[index], ContentAssetKind::Folder};
                constexpr float iconWidth = 24.0f;
                const float iconX = tile.X + (tile.Width - iconWidth) * 0.5f;
                const float iconY = tile.Y + 7.0f;
                const auto iconColor = missing ? theme.MissingText :
                    (selected || !asset ? theme.SelectionAccent : theme.TextSecondary);
                if (tile.Width >= iconWidth + 8.0f)
                    DrawAssetGlyph(window, renderer, {iconX, iconY}, iconColor, tileColor, label.Kind);

                constexpr float textPadding = 9.0f;
                const float textWidth = tile.Width - textPadding * 2.0f;
                const float measuredWidth = renderer.MeasureTextWidth(fontAtlas, label.Name);
                const float textX = tile.X + std::max(textPadding, (tile.Width - measuredWidth) * 0.5f);
                const auto textColor = missing ? theme.MissingText :
                    (selected ? theme.SelectionText : theme.TextSecondary);
                const float nameY = tile.Y + tile.Height - lineHeight * 2.0f - 5.0f;
                SidePanelDrawing::DrawText(window, renderer, fontAtlas,
                                           {textX, nameY}, textColor, textWidth, label.Name);
                const auto kind = missing ? std::string_view{"Missing"} : DescribeAssetKind(label.Kind);
                const float kindWidth = renderer.MeasureTextWidth(fontAtlas, kind);
                const float kindX = tile.X + std::max(textPadding, (tile.Width - kindWidth) * 0.5f);
                SidePanelDrawing::DrawText(window, renderer, fontAtlas,
                                           {kindX, nameY + lineHeight},
                                           missing ? theme.MissingText : theme.TextMuted, textWidth, kind);
            }
            renderer.SetClipRect(window, {rect.X, rect.Y}, {rect.Width, rect.Height});

            if (scrollLimit > 0.0f)
            {
                const float thumbHeight = std::min(gridRect.Height, std::max(12.0f,
                    gridRect.Height * gridRect.Height / (gridRect.Height + scrollLimit)));
                const float thumbY = gridRect.Y + m_PaintedScroll / scrollLimit * (gridRect.Height - thumbHeight);
                renderer.DrawRoundedRect(window, {gridRect.X + gridRect.Width + 3.0f, thumbY},
                                         {3.0f, thumbHeight}, theme.PanelBorder, 1.5f);
            }
        }
        renderer.SetClipRect(window, {}, {});
    }

    ContentBrowserAction ContentBrowserPanel::OnMousePressed(
        EditorSession &session,
        const float x,
        const float y)
    {
        if (!m_Rect.Contains(x, y))
        {
            m_ClickState.Reset();
            m_DragState.Cancel();
            return ContentBrowserAction::None;
        }

        if (m_ParentButtonRect.Contains(x, y))
        {
            const auto slash = m_CurrentFolder.rfind('/');
            session.ClearAssetSelection();
            ChangeFolder(slash == std::string::npos ? std::string{} : m_CurrentFolder.substr(0, slash));
            return ContentBrowserAction::FolderChanged;
        }

        if (m_PlaceButtonRect.Contains(x, y))
        {
            m_ClickState.Reset();
            m_DragState.Cancel();
            Limen::AssetRecord selected;
            if (!session.GetAssetRegistry().TryFind(session.GetSelectedAssetID(), selected) || !CanPlaceAsset(selected))
                return ContentBrowserAction::None;

            return m_PlaceCommandGuard.TryAcquire(
                       std::chrono::steady_clock::now())
                       ? ContentBrowserAction::PlaceAsset
                       : ContentBrowserAction::None;
        }

        const auto index = m_Grid.HitTest(x, y, m_PaintedScroll);
        if (!index)
        {
            m_ClickState.Reset();
            m_DragState.Cancel();
            return ContentBrowserAction::None;
        }

        const auto folderCount = m_ChildFolders.getSize();
        if (*index < folderCount)
        {
            auto folder = m_CurrentFolder;
            if (!folder.empty())
                folder += '/';
            folder += m_ChildFolders[*index];
            session.ClearAssetSelection();
            ChangeFolder(std::move(folder));
            return ContentBrowserAction::FolderChanged;
        }

        const auto &asset = m_Assets[m_AssetOrder[*index - folderCount]];
        const Limen::AssetID id = asset.ID;
        if (!session.SelectAsset(id))
        {
            m_ClickState.Reset();
            m_DragState.Cancel();
            return ContentBrowserAction::None;
        }

        const bool activate =
            m_ClickState.RegisterPress(id, std::chrono::steady_clock::now()) ==
            ContentBrowserClickState::Result::ActivateAsset;
        if (CanPlaceAsset(asset))
            m_DragState.Begin(id, x, y);
        else
            m_DragState.Cancel();

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

    bool ContentBrowserPanel::OnMouseScrolled(const float x, const float y, const float yOffset) noexcept
    {
        const float limit = m_Grid.ScrollLimit();
        if (!m_Grid.Viewport.Contains(x, y) || limit <= 0.0f ||
            !std::isfinite(yOffset) || yOffset == 0.0f || m_DragState.IsDragging())
            return false;

        constexpr float scrollPixelsPerStep = 44.0f;
        m_Scroll = std::clamp(m_Scroll - yOffset * scrollPixelsPerStep, 0.0f, limit);
        return true;
    }

    void ContentBrowserPanel::ResetLayout() noexcept
    {
        m_Rect = {};
        m_ParentButtonRect = {};
        m_PlaceButtonRect = {};
        m_Grid = {};
        m_PaintedScroll = 0.0f;
        m_ClickState.Reset();
        m_DragState.Cancel();
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
