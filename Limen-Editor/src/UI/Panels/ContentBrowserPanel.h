#pragma once

#include <chrono>

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
        ActivateAsset,
        PlaceAsset
    };

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

    // 资产行拖拽状态：按下后移动超过阈值才进入拖拽。
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

    // 左侧资产浏览器：只借用 Session 的 Registry 与资产选择。
    class ContentBrowserPanel final
    {
    public:
        // 每帧从 Registry 重建行数据；不创建 GPU 资源。
        void Update(const EditorSession &session);

        // 绘制标题与资产行；选中行使用行背景高亮。
        void Draw(
            const EditorSession &session,
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &fontAtlas,
            const EditorTheme &theme,
            const UIRect &rect
        );

        // 左键选择资产；同一行短时间内第二次点击表示激活（双击）。
        ContentBrowserAction OnMousePressed(
            EditorSession &session,
            float x,
            float y
        );

        void OnMouseMoved(float x, float y) noexcept;

        // 左键释放时返回本次是否真正发生了拖拽，并清空拖拽状态。
        bool EndDrag() noexcept;

        [[nodiscard]] bool IsDraggingAsset() const noexcept;

    private:
        UIRect m_Rect{};
        UIRect m_PlaceButtonRect{};
        Limen::LArray<Limen::AssetRecord> m_Rows;
        float m_FirstRowY = 0.0f;
        float m_RowHeight = 0.0f;
        ContentBrowserClickState m_ClickState;
        EditorCommandGuard m_PlaceCommandGuard;
        ContentBrowserDragState m_DragState;
    };
}
