#pragma once

#include <chrono>
#include <cstdint>

#include "UI/EditorCommandGuard.h"
#include "UI/EditorUILayout.h"

namespace Limen
{
    class Window;
    class FontAtlas;
}

namespace LimenEditor
{
    class EditorUIRenderer;
    struct EditorTheme;

    enum class PlacementToolbarAction : std::uint8_t
    {
        None,
        PlaceCube,
        PlacePointLight
    };

    struct PlacementToolbarLayout
    {
        UIRect Label{};
        UIRect CubeButton{};
        UIRect PointLightButton{};
    };

    // 顶栏 Place 工具区只描述按钮位置；对象创建仍由 EditorUILayer 发起。
    [[nodiscard]] inline PlacementToolbarLayout
    CalculatePlacementToolbarLayout(
        const UIRect &rect,
        const float rowHeight) noexcept
    {
        PlacementToolbarLayout layout{};
        if (rect.Width <= 0.0f || rect.Height <= 0.0f ||
            rowHeight <= 0.0f)
        {
            return layout;
        }

        constexpr float labelWidth = 34.0f;
        constexpr float gap = 6.0f;
        constexpr float cubeWidth = 59.0f;
        constexpr float pointLightWidth = 91.0f;

        const float rowY = rect.Y;
        if (rowHeight > rect.Height)
            return layout;

        const float totalWidth =
            labelWidth + gap + cubeWidth + gap + pointLightWidth;
        if (totalWidth > rect.Width)
            return layout;

        layout.Label = {
            rect.X,
            rowY,
            labelWidth,
            rowHeight
        };
        layout.CubeButton = {
            layout.Label.X + labelWidth + gap,
            rowY,
            cubeWidth,
            rowHeight
        };
        layout.PointLightButton = {
            layout.CubeButton.X + cubeWidth + gap,
            rowY,
            pointLightWidth,
            rowHeight
        };
        return layout;
    }

    class PlacementToolbarPanel final
    {
    public:
        void UpdateLayout(
            const UIRect &rect,
            const float rowHeight) noexcept
        {
            m_Layout = CalculatePlacementToolbarLayout(rect, rowHeight);
        }

        void Draw(
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &fontAtlas,
            const EditorTheme &theme,
            const UIRect &rect
        );

        // 点击返回一次放置意图；双击只返回第一次意图。
        [[nodiscard]] PlacementToolbarAction OnMousePressed(
            const float x,
            const float y,
            const std::chrono::steady_clock::time_point now) noexcept
        {
            if (m_Layout.CubeButton.Contains(x, y))
            {
                return m_CubeGuard.TryAcquire(now)
                           ? PlacementToolbarAction::PlaceCube
                           : PlacementToolbarAction::None;
            }
            if (m_Layout.PointLightButton.Contains(x, y))
            {
                return m_PointLightGuard.TryAcquire(now)
                           ? PlacementToolbarAction::PlacePointLight
                           : PlacementToolbarAction::None;
            }
            return PlacementToolbarAction::None;
        }

    private:
        PlacementToolbarLayout m_Layout{};
        EditorCommandGuard m_CubeGuard;
        EditorCommandGuard m_PointLightGuard;
    };
}
