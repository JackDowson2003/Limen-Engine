//
// Created by chenlong on 2026/8/12.
//
#pragma once
#include "RendererAPI.h"

namespace Limen
{
    class RendererCommand
    {
    public:
        inline static void Clear()
        {
            s_RendererAPI->Clear();
        }

        inline static void SetClearColor(const glm::vec4 &color)
        {
            s_RendererAPI->SetClearColor(color);
        }


        inline static void DrawIndexed(const std::shared_ptr<VertexArray> &vao)
        {
            s_RendererAPI->DrawIndexed(vao);
        }

        static RendererAPI *s_RendererAPI;
    };
}
