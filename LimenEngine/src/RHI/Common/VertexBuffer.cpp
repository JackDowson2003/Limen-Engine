//
// Created by chenlong on 2026/8/11.
//
#include "Limen/RHI/VertexBuffer.h"

#include "Limen/Core/Log.h"
#include "RHI/macOS/OpenGL/OpenGLVertexBuffer.h"
#include "Limen/Renderer/Renderer.h"

namespace Limen
{
    VertexBuffer* VertexBuffer::Create(const uint32_t size)
    {
        LM_CORE_ASSERT(
            size > 0,
            "VertexBuffer size must be greater than zero"
        );

        if (size == 0)
            return nullptr;

        switch (Renderer::GetRenderAPI())
        {
            case RendererAPI::API::NONE:
            {
                LM_CORE_ASSERT(
                    false,
                    "RendererAPI::API::NONE is not supported!"
                );
                return nullptr;
            }

            case RendererAPI::API::OPENGL:
                return new OpenGLVertexBuffer(size);

            default:
                break;
        }

        LM_CORE_ERROR(
            "The selected RendererAPI does not implement dynamic VertexBuffer creation"
        );
        return nullptr;
    }


    VertexBuffer *VertexBuffer::Create(const void *vertices, const uint32_t size)
    {
        LM_CORE_ASSERT(
            size > 0,
            "VertexBuffer size must be greater than zero"
        );

        if (size == 0)
            return nullptr;

        switch (Renderer::GetRenderAPI())
        {
            case RendererAPI::API::NONE:
            {
                LM_CORE_ASSERT(false, "RendererAPI::API::NONE is not supported!");
                return nullptr;
            }
            case RendererAPI::API::OPENGL: return new OpenGLVertexBuffer(vertices, size);
            default:
                break;
        }
        LM_CORE_ERROR("Unknown Render API! Cannot create vertex buffer!");
        return nullptr;
    }
}
