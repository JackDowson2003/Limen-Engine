//
// Created by chenlong on 2026/10/1.
//

#include "Limen/Renderer/GPUProfiler.h"

#include "Limen/Core/Core.h"
#include "Limen/Core/Log.h"

#include "Limen/RHI/RendererAPI.h"


#if defined(LIMEN_PLATFORM_MACOS)
#include "RHI/macOS/OpenGL/OpenGLGPUProfiler.h"
#endif

namespace Limen
{
    Scope<GPUProfiler> GPUProfiler::Create()
    {
        switch (RendererAPI::GetAPI())
        {
            case RendererAPI::API::OPENGL:
            {
                #if defined(LIMEN_PLATFORM_MACOS)
                    return CreateScope<OpenGLGPUProfiler>();
                #else
                    LM_CORE_ERROR("OpenGL GPUProfiler is unavailable on this platform");
                    return nullptr;
                #endif
            }
            case RendererAPI::API::NONE:
                LM_CORE_ERROR("Cannot create GPUProfiler with RendererAPI::NONE");
                return nullptr;

            default:
                LM_CORE_ERROR("GPUProfiler is not implemented for the selected RendererAPI");
                return nullptr;
        }
    }
}
