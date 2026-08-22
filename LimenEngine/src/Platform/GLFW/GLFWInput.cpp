//
// Created by chenlong on 2026/8/8.
//
#include "GLFWInput.h"

#include <GLFW/glfw3.h>

#include "Limen/Application/Application.h"
#include "Platform/GLFW/GLFWKeyCodes.h"

namespace Limen
{
#ifdef LIMEN_PLATFORM_MACOS
    Input *Input::s_Instance = new GLFWInput();
#endif

    static int ToGLFWMouseButton(const MouseButton button)
    {
        switch (button)
        {
            case MouseButton::Left: return GLFW_MOUSE_BUTTON_LEFT;
            case MouseButton::Right: return GLFW_MOUSE_BUTTON_RIGHT;
            case MouseButton::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
            case MouseButton::Button4: return GLFW_MOUSE_BUTTON_4;
            case MouseButton::Button5: return GLFW_MOUSE_BUTTON_5;
        }

        return -1;
    }

    GLFWInput::~GLFWInput() = default;

    bool GLFWInput::IsKeyPressedImpl(const KeyCode keyCode) const
    {
        const auto &window = Application::GetApp().GetWindow();
        const auto macWindow = window.GetNativeWindow();
        const int glfwKey = GLFWKeyFromKeyCode(keyCode);
        if (glfwKey == GLFW_KEY_UNKNOWN)
            return false;

        const int key = glfwGetKey(static_cast<GLFWwindow *>(macWindow), glfwKey);
        return key == GLFW_PRESS || key == GLFW_REPEAT;
    }

    std::pair<float, float> GLFWInput::GetMousePosImpl() const
    {
        const auto &window = Application::GetApp().GetWindow();
        const auto macWindow = window.GetNativeWindow();
        double mouseX;
        double mouseY;
        glfwGetCursorPos(static_cast<GLFWwindow *>(macWindow), &mouseX, &mouseY);
        return {static_cast<float>(mouseX), static_cast<float>(mouseY)};
    }

    bool GLFWInput::IsMouseButtonPressedImpl(const MouseButton button) const
    {
        const auto &window = Application::GetApp().GetWindow();
        const auto macWindow = window.GetNativeWindow();
        const int glfwButton = ToGLFWMouseButton(button);
        if (glfwButton < 0)
            return false;

        return glfwGetMouseButton(static_cast<GLFWwindow *>(macWindow), glfwButton) == GLFW_PRESS;
    }

    float GLFWInput::GetMouseXImpl() const
    {
        const auto [x, y] = GetMousePosImpl();
        return x;
    }

    float GLFWInput::GetMouseYImpl() const
    {
        const auto [x, y] = GetMousePosImpl();
        return y;
    }

    void GLFWInput::SetCursorModeImpl(const CursorMode mode) const
    {
        const auto &window = Application::GetApp().GetWindow();
        auto *nativeWindow = static_cast<GLFWwindow *>(window.GetNativeWindow());

        switch (mode)
        {
            case CursorMode::Normal:
                glfwSetInputMode(nativeWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                break;

            /**
            *光标不可见。
             鼠标位置仍按照普通桌面光标处理。
             仍然会受到窗口或屏幕边界限制。
             到达屏幕边缘后不能继续旋转。
             */
            case CursorMode::Hidden:
                glfwSetInputMode(nativeWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                break;

            /**
            *光标不可见。
             光标被当前窗口捕获。
             不再受到屏幕边缘限制。
             GLFW 会提供连续的虚拟鼠标位置。
             可以持续获得 Mouse Delta。
             适合 UE 风格编辑器相机和第一人称游戏。
             */
            case CursorMode::Locked:
                glfwSetInputMode(nativeWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                break;
        }
    }
}
