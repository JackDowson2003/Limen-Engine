//
// Created by chenlong on 2026/8/11.
//
#include "Limen/RHI/Buffer.h"

#include "Limen/Core/Log.h"
#include "RHI/macOS/OpenGL/OpenGLVertexBuffer.h"
#include "Limen/Renderer/Renderer.h"

namespace Limen
{

    VertexBuffer *VertexBuffer::Create(const void *vertices, const uint32_t size)
    {
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


    IndexBuffer *IndexBuffer::Create(const uint32_t *indices, const uint32_t count)
    {
        switch (Renderer::GetRenderAPI())
        {
            case RendererAPI::API::NONE:
            {
                LM_CORE_ASSERT(false, "RendererAPI::API::NONE is not supported!");
                return nullptr;
            }
            case RendererAPI::API::OPENGL: return new OpenGLIndexBuffer(indices, count);
            default:
                break;
        }
        LM_CORE_ERROR("Unknown Render API! Cannot create index buffer!");
        return nullptr;
    }
}
