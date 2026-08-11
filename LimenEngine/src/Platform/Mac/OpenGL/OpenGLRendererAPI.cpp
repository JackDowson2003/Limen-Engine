//
// Created by chenlong on 2026/8/12.
//

#include <glad/gl.h>
#include "OpenGLRendererAPI.h"

namespace Limen
{
    OpenGLRendererAPI::~OpenGLRendererAPI()
    {
    }

    inline void OpenGLRendererAPI::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    inline void OpenGLRendererAPI::SetClearColor(const glm::vec4 &color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    inline void OpenGLRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray> &vertexArray)
    {
        glDrawElements(GL_TRIANGLES,
                       static_cast<int>(vertexArray->GetIndexBuffer()->GetCount()),
                       GL_UNSIGNED_INT,
                       nullptr);
    }
}
