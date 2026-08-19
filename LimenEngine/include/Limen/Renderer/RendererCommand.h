//
// Created by chenlong on 2026/8/12.
//
#pragma once
#include "RendererAPI.h"

namespace Limen
{
    class LIMEN_API RendererCommand
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

        static void Init()
        {
            s_RendererAPI->Init();
        }


        inline static void DrawIndexed(const VertexArray &vao)
        {
            s_RendererAPI->DrawIndexed(vao); //我们的API对应的方法
        }

        static RendererAPI *s_RendererAPI;
    };
}
