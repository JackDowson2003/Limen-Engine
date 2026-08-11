//
// Created by chenlong on 2026/8/12.
//
#include "Renderer/RendererAPI.h"

namespace Limen
{
#if defined(LIMEN_PLATFORM_MACOS)
     RendererAPI::API RendererAPI::s_API = RendererAPI::API::OPENGL;
#endif

}