//
// Created by chenlong on 2026/8/12.
//

#pragma once
#include "Renderer/RendererAPI.h"

namespace Limen
{
    class OpenGLRendererAPI : public RendererAPI
    {
    public:
        ~OpenGLRendererAPI() override;

         void Clear()  override;
         void SetClearColor(const glm::vec4& color) override;

         void DrawIndexed(const VertexArray& vertexArray)  override;
    };


}
