//
// Created by chenlong on 2026/8/7.
//
#include "Window.h"
#include "Paltform/Mac/MacWindow.h"

namespace Limen
{
#if  defined(LIMEN_PLATFORM_MACOS)
    Window *Window::Create(const WindowProps &props)
    {
        return new MacWindow(props);
    }
#else
    #error "Platform does not support Mac OS 、Windows and Linux."
#endif
}
