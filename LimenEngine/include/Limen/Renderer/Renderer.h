//
// Created by chenlong on 2026/8/11.
//
#pragma once
#include "Limen/Renderer/RendererCommand.h"
#include "Limen/RHI/Shader.h"
#include "Limen/Core/Core.h"
#include "Limen/Renderer/Camera.h"

namespace Limen
{
    class LIMEN_API Renderer
    {
    public:
        // BeginScene/EndScene 定义一个逻辑上的渲染提交区间。
        // 场景开始后才能调用 Submit，且同一时间只能存在一个场景。
        static void BeginScene(const Camera& camera);

        /**
         * @brief 更新Renderer输出使用的GPU Viewport。
         *
         * 该函数不修改Camera的投影矩阵；CameraController会独立处理宽高比。
         */
        static void OnWindowResize(uint32_t width, uint32_t height);

        static void EndScene();

        static void Init();

        /**
         * @brief 释放Renderer持有的后端对象。
         *
         * 调用时OpenGL Context仍然必须有效。
         */
        static void Shutdown();

        //OpenGL Submit
        static void Submit(
            const Ref<Shader> &shader,
            const VertexArray &vertexArray,
            const glm::mat4 &transform = glm::mat4(1.0f)
        );

        static RendererAPI::API GetRenderAPI()
        {
            return RendererAPI::GetAPI();
        }
    };
}
