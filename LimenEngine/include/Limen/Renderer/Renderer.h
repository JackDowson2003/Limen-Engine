//
// Created by chenlong on 2026/8/11.
//
#pragma once

#include "Limen/Core/Core.h"
#include "Limen/Renderer/Camera.h"
#include "Limen/Renderer/RendererCommand.h"
#include "Limen/RHI/GraphicsPipeline.h"
#include "Limen/RHI/Shader.h"

namespace Limen
{
    class LIMEN_API Renderer
    {
    public:
        /**
         * @brief 开始逻辑场景提交区间，并缓存本帧相机数据。
         *
         * BeginScene() 与 EndScene() 必须成对调用；同一时刻只能有一个活动场景。
         */
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

        /**
         * @brief 旧的 Shader 直接提交入口，保留到 2D 示例迁移完成。
         *
         * 新代码应优先使用 GraphicsPipeline 重载，由 Pipeline 统一管理固定状态。
         */
        static void Submit(
            const Ref<Shader> &shader,
            const VertexArray &vertexArray,
            const glm::mat4 &transform = glm::mat4(1.0f)
        );

        /**
         * @brief 使用完整 GraphicsPipeline 提交一次索引绘制。
         *
         * @param pipeline
         * 本次绘制使用的 Shader 和固定功能状态。
         *
         * @param vertexArray
         * 本次绘制使用的顶点和索引数据。
         *
         * @param transform
         * 当前物体从模型空间变换到世界空间的矩阵。
         *
         * 调用顺序为：绑定 Pipeline、上传绘制参数、绑定几何数据、发出绘制命令。
         */
        static void Submit(
            const GraphicsPipeline& pipeline,
            const VertexArray& vertexArray,
            const glm::mat4& transform = glm::mat4(1.0f)
        );



        static RendererAPI::API GetRenderAPI()
        {
            return RendererAPI::GetAPI();
        }
    };
}
