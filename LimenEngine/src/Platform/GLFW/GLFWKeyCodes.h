#pragma once

#include "Limen/Input/KeyCodes.h"

namespace Limen
{
    // GLFW 是窗口后端实现细节；这个函数不得被公共 include/Limen API 引用。
    [[nodiscard]] KeyCode KeyCodeFromGLFW(int glfwKey);
    [[nodiscard]] int GLFWKeyFromKeyCode(KeyCode keyCode);
}
