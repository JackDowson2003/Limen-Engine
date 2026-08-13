//
// Created by chenlong on 2026/8/11.
//
#pragma once
#include "RendererCommand.h"
#include "Shader.h"
#include "Core.h"
#include "OrthoGraphicCamera.h"

namespace Limen
{

    class LIMEN_API Renderer
    {
    public:
        // BeginScene/EndScene 定义一个逻辑上的渲染提交区间。
        // 场景开始后才能调用 Submit，且同一时间只能存在一个场景。
        static void BeginScene(const OrthoGraphicCamera& camera);
        static void EndScene();

        static void Submit(
             Shader& shader,
            const VertexArray& vertexArray
        );

        inline static RendererAPI::API GetRenderAPI()
        {
            return RendererAPI::GetAPI();
        }
    };
}
