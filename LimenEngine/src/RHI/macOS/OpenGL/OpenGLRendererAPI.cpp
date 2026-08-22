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

    void OpenGLRendererAPI::Init()
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    inline void OpenGLRendererAPI::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    inline void OpenGLRendererAPI::SetClearColor(const glm::vec4 &color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void OpenGLRendererAPI::SetDepthTest(const bool enabled)
    {
        if (enabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }

    inline void OpenGLRendererAPI::DrawIndexed(const VertexArray &vertexArray)
    {
        glDrawElements(GL_TRIANGLES,
                       static_cast<int>(vertexArray.GetIndexBuffer()->GetCount()),
                       GL_UNSIGNED_INT,
                       nullptr);
    }

    void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        glViewport(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height));
    }
}
