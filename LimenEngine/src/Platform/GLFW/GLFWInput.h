//
// Created by chenlong on 2026/8/8.
//
#pragma once
#include "Limen/Input/Input.h"

namespace Limen
{
    /**
     * @brief 通过GLFW查询键盘和鼠标即时状态。
     *
     * 该实现依赖窗口库而不是macOS原生API，因此属于Platform/GLFW。
     */
    class GLFWInput final : public Input
    {
    public:
        GLFWInput() = default;

        ~GLFWInput() override;

    protected:
        [[nodiscard]] bool IsKeyPressedImpl(KeyCode keyCode) const override;

        [[nodiscard]] std::pair<float,float> GetMousePosImpl() const override;

        [[nodiscard]] bool IsMouseButtonPressedImpl(MouseButton button) const override;

        [[nodiscard]] float GetMouseXImpl() const override;

        [[nodiscard]] float GetMouseYImpl() const override;

        void SetCursorModeImpl(CursorMode mode) const override;
    };
}
