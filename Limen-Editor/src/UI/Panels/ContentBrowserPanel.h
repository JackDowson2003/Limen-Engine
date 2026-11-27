#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "Limen/Asset/AssetRegistry.h"
#include "Limen/Core/Core.h"
#include "UI/EditorCommandGuard.h"
#include "UI/EditorUILayout.h"

namespace Limen
{
    class Window;
    class FontAtlas;
}

namespace LimenEditor
{
    class EditorSession;
    class EditorUIRenderer;
    struct EditorTheme;

    enum class ContentBrowserAction
    {
        None,
        SelectionChanged,
        FolderChanged,
        ActivateAsset,
        PlaceAsset
    };

    enum class ContentAssetKind : std::uint8_t
    {
        File,
        Mesh,
        Scene,
        Image,
        Material,
        Folder
    };

    struct ContentAssetLabel
    {
        std::string_view Name;
        ContentAssetKind Kind;
    };

    // Registry 路径使用 / 分隔。只有完整的目录分量匹配时才返回剩余路径。
    [[nodiscard]] inline std::string_view RelativeContentPath(
        const std::string_view path, const std::string_view folder) noexcept
    {
        if (folder.empty())
            return path;
        if (path.size() <= folder.size() || !path.starts_with(folder) || path[folder.size()] != '/')
            return {};
        return path.substr(folder.size() + 1);
    }

    // 文件名用于卡片标注；完整资产路径仍由选中项显示。
    [[nodiscard]] inline ContentAssetLabel DescribeContentAssetPath(const std::string_view path) noexcept
    {
        const auto slash = path.rfind('/');
        const auto filename = slash == std::string_view::npos ? path : path.substr(slash + 1U);
        const auto dot = filename.rfind('.');
        if (dot == 0 || dot == std::string_view::npos)
            return {filename, ContentAssetKind::File};

        const auto extension = filename.substr(dot);
        const auto isExtension = [extension](const std::string_view expected)
        {
            return std::equal(extension.begin(), extension.end(), expected.begin(), expected.end(),
                [](const char value, const char lower)
                {
                    const char folded = value >= 'A' && value <= 'Z'
                        ? static_cast<char>(value + ('a' - 'A')) : value;
                    return folded == lower;
                });
        };
        const auto stem = filename.substr(0, dot);
        if (isExtension(".obj"))
            return {stem, ContentAssetKind::Mesh};
        if (isExtension(".lscene"))
            return {stem, ContentAssetKind::Scene};
        if (isExtension(".png") || isExtension(".jpg") || isExtension(".jpeg") || isExtension(".tga"))
            return {stem, ContentAssetKind::Image};
        if (isExtension(".mtl"))
            return {stem, ContentAssetKind::Material};
        return {filename, ContentAssetKind::File};
    }

    // 手动双击检测：一次激活后清空状态，避免连续按下重复触发。
    class ContentBrowserClickState final
    {
    public:
        enum class Result
        {
            SelectionChanged,
            ActivateAsset
        };

        [[nodiscard]] Result RegisterPress(
            const Limen::AssetID id,
            const std::chrono::steady_clock::time_point now) noexcept
        {
            constexpr auto doubleClickInterval = std::chrono::milliseconds(350);
            const bool sameAsset = m_LastClickedAsset.isValid() &&
                                   m_LastClickedAsset == id;
            const bool withinInterval =
                now >= m_LastClickTime &&
                now - m_LastClickTime <= doubleClickInterval;

            if (sameAsset && withinInterval)
            {
                Reset();
                return Result::ActivateAsset;
            }

            m_LastClickedAsset = id;
            m_LastClickTime = now;
            return Result::SelectionChanged;
        }

        void Reset() noexcept
        {
            m_LastClickedAsset = {};
            m_LastClickTime = {};
        }

    private:
        Limen::AssetID m_LastClickedAsset{};
        std::chrono::steady_clock::time_point m_LastClickTime{};
    };

    // 资产拖拽状态：按下后移动超过阈值才进入拖拽。
    class ContentBrowserDragState final
    {
    public:
        void Begin(
            const Limen::AssetID id,
            const float x,
            const float y) noexcept
        {
            m_AssetID = id;
            m_StartX = x;
            m_StartY = y;
            m_PointerDown = true;
            m_IsDragging = false;
        }

        void Update(const float x, const float y) noexcept
        {
            if (!m_PointerDown || m_IsDragging)
                return;

            constexpr float dragThreshold = 5.0f;
            const float deltaX = x - m_StartX;
            const float deltaY = y - m_StartY;
            if (deltaX * deltaX + deltaY * deltaY >=
                dragThreshold * dragThreshold)
                m_IsDragging = true;
        }

        [[nodiscard]] bool End() noexcept
        {
            const bool wasDragging = m_PointerDown && m_IsDragging;
            Cancel();
            return wasDragging;
        }

        void Cancel() noexcept
        {
            m_AssetID = {};
            m_StartX = 0.0f;
            m_StartY = 0.0f;
            m_PointerDown = false;
            m_IsDragging = false;
        }

        [[nodiscard]] bool IsDragging() const noexcept
        {
            return m_PointerDown && m_IsDragging;
        }

    private:
        Limen::AssetID m_AssetID{};
        float m_StartX = 0.0f;
        float m_StartY = 0.0f;
        bool m_PointerDown = false;
        bool m_IsDragging = false;
    };

    // 绘制和命中共用的平铺布局。位置和滚动距离均为逻辑像素。
    struct ContentBrowserGrid
    {
        static constexpr float TileHeight = 78.0f;
        static constexpr float Gap = 8.0f;
        static constexpr float RowPitch = TileHeight + Gap;

        UIRect Viewport{};
        std::uint32_t AssetCount = 0;
        std::uint32_t Columns = 0;
        float TileWidth = 0.0f;

        [[nodiscard]] static ContentBrowserGrid Calculate(
            const UIRect &viewport, const std::uint32_t assetCount) noexcept
        {
            if (viewport.Width <= 0.0f || viewport.Height <= 0.0f)
                return {};

            constexpr float preferredWidth = 145.0f;
            const auto availableColumns = static_cast<std::uint32_t>((viewport.Width + Gap) / (preferredWidth + Gap));
            return {viewport, assetCount, std::max(1U, availableColumns), std::min(preferredWidth, viewport.Width)};
        }

        [[nodiscard]] float ScrollLimit() const noexcept
        {
            if (Columns == 0 || AssetCount == 0)
                return 0.0f;
            const auto lastRow = (AssetCount - 1U) / Columns;
            const float contentHeight = static_cast<float>(lastRow) * RowPitch + TileHeight;
            return std::max(0.0f, contentHeight - Viewport.Height);
        }

        [[nodiscard]] UIRect TileAt(const std::uint32_t index, const float scroll) const noexcept
        {
            if (Columns == 0 || index >= AssetCount)
                return {};
            return {
                Viewport.X + static_cast<float>(index % Columns) * (TileWidth + Gap),
                Viewport.Y + static_cast<float>(index / Columns) * RowPitch - scroll,
                TileWidth,
                TileHeight
            };
        }

        [[nodiscard]] std::optional<std::uint32_t> HitTest(
            const float x, const float y, const float scroll) const noexcept
        {
            if (Columns == 0 || AssetCount == 0 || !Viewport.Contains(x, y))
                return std::nullopt;

            const auto column = static_cast<std::uint32_t>((x - Viewport.X) / (TileWidth + Gap));
            const auto row = static_cast<std::uint32_t>((y - Viewport.Y + scroll) / RowPitch);
            if (column >= Columns || row > (AssetCount - 1U) / Columns)
                return std::nullopt;
            const std::uint32_t firstInRow = row * Columns;
            if (column >= AssetCount - firstInRow)
                return std::nullopt;
            const std::uint32_t index = firstInRow + column;
            return TileAt(index, scroll).Contains(x, y) ? std::optional{index} : std::nullopt;
        }
    };

    // 底部资产托盘：只借用 Session 的 Registry 与资产选择。
    class ContentBrowserPanel final
    {
    public:
        // 刷新当前目录的子目录与 Registry 资源；列表或目录变化时返回 true。
        bool Update(const EditorSession &session);
        void AppendVisibleLabels(std::string &text) const;

        // 绘制标题和可滚动资产网格。布局记录用于后续点击命中。
        void Draw(
            const EditorSession &session,
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &fontAtlas,
            const EditorTheme &theme,
            const UIRect &rect
        );

        // 左键打开目录或选择资产，同一资源短时间内第二次点击表示激活（双击）。
        ContentBrowserAction OnMousePressed(
            EditorSession &session,
            float x,
            float y
        );

        void OnMouseMoved(float x, float y) noexcept;
        [[nodiscard]] bool OnMouseScrolled(float x, float y, float yOffset) noexcept;

        // 隐藏或调整尺寸后清空命中区域，保留资源浏览进度。
        void ResetLayout() noexcept;

        // 左键释放时返回本次是否真正发生了拖拽，并清空拖拽状态。
        bool EndDrag() noexcept;

        [[nodiscard]] bool IsDraggingAsset() const noexcept;

    private:
        void ChangeFolder(std::string logicalPath);

        UIRect m_Rect{};
        UIRect m_ParentButtonRect{};
        UIRect m_PlaceButtonRect{};
        std::filesystem::path m_AssetRoot;
        std::string m_CurrentFolder;
        Limen::LArray<std::string> m_DiskFolders;
        Limen::LArray<std::string> m_ChildFolders;
        std::chrono::steady_clock::time_point m_NextDirectoryRefresh{};
        bool m_FolderChanged = true;
        Limen::LArray<Limen::AssetRecord> m_Assets;
        Limen::LArray<std::uint32_t> m_AssetOrder;
        ContentBrowserGrid m_Grid{};
        float m_Scroll = 0.0f;
        float m_PaintedScroll = 0.0f;
        ContentBrowserClickState m_ClickState;
        EditorCommandGuard m_PlaceCommandGuard;
        ContentBrowserDragState m_DragState;
    };
}
