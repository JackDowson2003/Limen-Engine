//
// Created by chenlong on 2026/8/11.
//
#include "Limen/RHI/VertexArray.h"

#include "Limen/Core/Log.h"
#include "RHI/macOS/OpenGL/OpenGLVertexArray.h"
#include "Limen/Renderer/Renderer.h"

namespace Limen
{
    VertexArray * VertexArray::Create()
    {
        switch (Renderer::GetRenderAPI())
        {
            case RendererAPI::API::NONE:
            {
                LM_CORE_ASSERT(false, "RendererAPI::API::NONE is not supported!");
                return nullptr;
            }
            case RendererAPI::API::OPENGL: return new OpenGLVertexArray();
            default:
                break;
        }
        LM_CORE_ERROR("Unknown Render API! Cannot create vertex buffer!");
        return nullptr;
    }
}
