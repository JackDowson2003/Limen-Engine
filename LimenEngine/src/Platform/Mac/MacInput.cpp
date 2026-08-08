//
// Created by chenlong on 2026/8/8.
//
#include "MacInput.h"

#include <GLFW/glfw3.h>

#include "Application.h"
#include "MacWindow.h"
#include "Platform/GLFW/GLFWKeyCodes.h"

namespace Limen
{
#ifdef LIMEN_PLATFORM_MACOS
    Input *Input::s_Instance = new MacInput();
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

    MacInput::~MacInput() = default;

    bool MacInput::IsKeyPressedImpl(const KeyCode keyCode) const
    {
        const auto &window = Application::GetApp().GetWindow();
        const auto macWindow = window.GetNativeWindow();
        const int glfwKey = GLFWKeyFromKeyCode(keyCode);
        if (glfwKey == GLFW_KEY_UNKNOWN)
            return false;

        const int key = glfwGetKey(static_cast<GLFWwindow *>(macWindow), glfwKey);
        return key == GLFW_PRESS || key == GLFW_REPEAT;
    }

    std::pair<float, float> MacInput::GetMousePosImpl() const
    {
        const auto &window = Application::GetApp().GetWindow();
        const auto macWindow = window.GetNativeWindow();
        double mouseX;
        double mouseY;
        glfwGetCursorPos(static_cast<GLFWwindow *>(macWindow), &mouseX, &mouseY);
        return {static_cast<float>(mouseX), static_cast<float>(mouseY)};
    }

    bool MacInput::IsMouseButtonPressedImpl(const MouseButton button) const
    {
        const auto &window = Application::GetApp().GetWindow();
        const auto macWindow = window.GetNativeWindow();
        const int glfwButton = ToGLFWMouseButton(button);
        if (glfwButton < 0)
            return false;

        return glfwGetMouseButton(static_cast<GLFWwindow *>(macWindow), glfwButton) == GLFW_PRESS;
    }

    float MacInput::GetMouseXImpl() const
    {
        const auto [x, y] = GetMousePosImpl();
        return x;
    }

    float MacInput::GetMouseYImpl() const
    {
        const auto [x, y] = GetMousePosImpl();
        return y;
    }
}
