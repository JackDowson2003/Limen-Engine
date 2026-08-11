//
// Created by chenlong on 2026/8/11.
//
#pragma once
#include "RendererCommand.h"
#include "Shader.h"

namespace Limen
{

    class Renderer
    {
    public:

        static void BeginScene();
        static void EndScene();

        static void Submit(const std::shared_ptr<VertexArray>& vertexArray);

        inline static RendererAPI::API GetRenderAPI()
        {
            return RendererAPI::GetAPI();
        }
    };
}
