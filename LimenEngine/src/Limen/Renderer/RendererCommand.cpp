//
// Created by chenlong on 2026/8/12.
//
#include "Renderer/RendererCommand.h"

#include "Platform/Mac/OpenGL/OpenGLRendererAPI.h"

namespace Limen
{
#if  defined(LIMEN_PLATFORM_MACOS) || defined(LIMEN_PLATFORM_LINUX)
    RendererAPI *RendererCommand::s_RendererAPI = new OpenGLRendererAPI();
#endif
}
