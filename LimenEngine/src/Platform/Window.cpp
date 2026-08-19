//
// Created by chenlong on 2026/8/7.
//
#include "Window.h"

#include "Log.h"
#include "Platform/Mac/MacWindow.h"
#include "Renderer/Renderer.h"

namespace Limen
{
    Window *Window::Create(const WindowProps &props)
    {
        switch (Renderer::GetRenderAPI())
        {
            case RendererAPI::API::NONE:
            {
                LM_CORE_ERROR("Platform does not support Mac OS 、Windows and Linux.");
                return nullptr;
            }
            case RendererAPI::API::OPENGL:
                return new MacWindow(props);
            default:
            {
                LM_CORE_ERROR("Don't have this version API");
                return nullptr;
            }

        }
    }
}
