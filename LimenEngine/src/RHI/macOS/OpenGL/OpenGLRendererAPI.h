//
// Created by chenlong on 2026/8/12.
//

#pragma once
#include "Limen/RHI/RendererAPI.h"

namespace Limen
{
    class OpenGLRendererAPI : public RendererAPI
    {
    public:
        ~OpenGLRendererAPI() override;

        void Init() override;

        void Clear() override;

        void SetClearColor(const glm::vec4 &color) override;

        /**
         * @brief 开启或关闭深度测试。
         *
         * 深度测试用于比较当前片元与深度缓冲中已有片元的深度，
         * 决定当前片元是否被更近的物体遮挡。
         *
         * @param enabled
         * true：开启深度测试，通常用于3D场景。
         * false：关闭深度测试，通常用于屏幕空间2D覆盖层和UI。
         */
        void SetDepthTest(bool enabled) override;

        /**
         * @brief 使用索引缓冲提交一次绘制。
         *
         * @param vertexArray
         * 保存顶点缓冲、索引缓冲以及顶点布局的 VertexArray。
         *
         * @param topology
         * GPU 使用的图元组装方式，例如 TriangleList。
         *
         * @param indexCount
         * 本次实际绘制的索引数量。
         * 传入0表示使用 IndexBuffer 保存的全部索引。
         */
        void DrawIndexed(
           const VertexArray& vertexArray,
           PrimitiveTopology topology,
           uint32_t indexCount = 0
       ) override;

        void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
    };
}
