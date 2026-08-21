//
// Created by chenlong on 2026/8/7.
//
#include "Limen/Application/Window.h"

#include "Limen/Core/Log.h"
#if defined(LIMEN_PLATFORM_MACOS)
#include "Platform/macOS/MacWindow.h"
#endif

namespace Limen
{
    Window *Window::Create(const WindowProps &props)
    {
#if defined(LIMEN_PLATFORM_MACOS)
        // Window只按操作系统选择实现，不关心最终使用OpenGL还是Metal。
        return new MacWindow(props);
#elif defined(LIMEN_PLATFORM_WINDOWS)
        LM_CORE_ERROR("WindowsWindow is not implemented yet");
        return nullptr;
#elif defined(LIMEN_PLATFORM_LINUX)
        LM_CORE_ERROR("LinuxWindow is not implemented yet");
        return nullptr;
#else
        LM_CORE_ERROR("The current operating system is not supported");
        return nullptr;
#endif
    }
}
