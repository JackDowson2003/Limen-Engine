//
// Created by chenlong on 2026/8/8.
//

#pragma once

#include <cstdint>
#include <utility>

#include "Limen/Core/Core.h"
#include "Limen/Input/KeyCodes.h"


namespace Limen
{
    // 引擎统一的鼠标按键，不依赖 GLFW_MOUSE_BUTTON_*。
    enum class MouseButton : std::uint8_t
    {
        Left,
        Right,
        Middle,
        Button4,
        Button5
    };

    /**
     * @brief 引擎统一的鼠标光标模式。
     *
     * Normal：显示光标并允许它自由移动；
     * Hidden：隐藏光标，但仍受窗口边界限制；
     * Locked：隐藏并捕获光标，适合编辑器相机和第一人称视角。
     */
    enum class CursorMode : std::uint8_t
    {
        Normal,
        Hidden,
        Locked
    };

    class LIMEN_API Input
    {
    public:
        virtual ~Input() = default;

        Input() = default;

        [[nodiscard]] static bool IsKeyPressed(KeyCode keyCode)
        {
            return s_Instance->IsKeyPressedImpl(keyCode);
        }

        [[nodiscard]] static bool IsMouseButtonPressed(MouseButton button)
        {
            return s_Instance->IsMouseButtonPressedImpl(button);
        }

        [[nodiscard]] static std::pair<float, float> GetMousePos()
        {
            return s_Instance->GetMousePosImpl();
        }

        [[nodiscard]] static float GetMouseX() { return s_Instance->GetMouseXImpl(); }
        [[nodiscard]] static float GetMouseY() { return s_Instance->GetMouseYImpl(); }

        static void SetCursorMode(const CursorMode mode)
        {
            s_Instance->SetCursorModeImpl(mode);
        }



    protected:
        [[nodiscard]] virtual bool IsKeyPressedImpl(KeyCode keyCode) const = 0;
        [[nodiscard]] virtual bool IsMouseButtonPressedImpl(MouseButton button) const = 0;
        [[nodiscard]] virtual std::pair<float, float> GetMousePosImpl() const = 0;
        [[nodiscard]] virtual float GetMouseXImpl() const = 0;
        [[nodiscard]] virtual float GetMouseYImpl() const = 0;

        // 平台后端负责把公共 CursorMode 转换为原生窗口系统的光标模式。
        virtual void SetCursorModeImpl(CursorMode mode) const = 0;

    private:
        static Input* s_Instance;

    };
}
