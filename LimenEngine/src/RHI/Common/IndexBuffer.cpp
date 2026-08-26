//
// Created by chenlong on 2026/8/26.
//

#include "Limen/RHI/IndexBuffer.h"

#include "Limen/Core/Log.h"
#include "Limen/Renderer/Renderer.h"

#if defined(LIMEN_PLATFORM_MACOS)
    #include "RHI/macOS/OpenGL/OpenGLIndexBuffer.h"
#endif

namespace Limen
{
    IndexBuffer *IndexBuffer::Create(
        const uint32_t *indices,
        const uint32_t count
    )
    {
        switch (Renderer::GetRenderAPI())
        {
            case RendererAPI::API::NONE:
            {
                LM_CORE_ASSERT(
                    false,
                    "RendererAPI::API::NONE is not supported"
                );
                return nullptr;
            }

            case RendererAPI::API::OPENGL:
            {
#if defined(LIMEN_PLATFORM_MACOS)
                return new OpenGLIndexBuffer(indices, count);
#else
                LM_CORE_ERROR(
                    "The OpenGL IndexBuffer backend is not available on this platform"
                );
                return nullptr;
#endif
            }

            default:
                break;
        }

        LM_CORE_ERROR(
            "The selected RendererAPI does not implement IndexBuffer creation"
        );
        return nullptr;
    }
}
