//
// Created by chenlong on 2026/8/11.
//
#include "Renderer/Renderer.h"

namespace Limen
{
    void Renderer::BeginScene()
    {
    }

    void Renderer::EndScene()
    {
    }

    void Renderer::Submit(const std::shared_ptr<VertexArray> &vertexArray)
    {
        vertexArray->Bind();
        RendererCommand::DrawIndexed(vertexArray);
    }
}
