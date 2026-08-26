//
// Created by chenlong on 2026/8/12.
//
#pragma once

#include "Limen/RHI/RendererAPI.h"

namespace Limen
{
    class LIMEN_API RendererCommand
    {
    public:
        static void Clear()
        {
            if (RendererAPI *api = GetRendererAPI())
                api->Clear();
        }

        static void SetClearColor(const glm::vec4 &color)
        {
            if (RendererAPI *api = GetRendererAPI())
                api->SetClearColor(color);
        }

        /**
         * @brief 根据RendererAPI::GetAPI()和当前平台创建并初始化后端。
         * 初始化当前选定图形 API 的底层命令后端，例如 OpenGLRendererAPI，并设置初始 GPU 状态。
         */
        static void Init();

        /**
         * @brief 释放当前RendererAPI后端。
         *
         * 必须在图形Context/Window销毁前调用。
         */
        static void Shutdown();

        /**
         * @brief 向当前RendererAPI发送深度测试状态。
         *
         * @param enabled
         * true表示开启，false表示关闭。
         */
        static void SetDepthTest(const bool enabled)
        {
            if (RendererAPI *api = GetRendererAPI())
                api->SetDepthTest(enabled);
        }

        /**
         * @brief 向当前 RendererAPI 提交一次索引绘制命令。
         *
         * @param vertexArray 顶点输入布局以及顶点、索引缓冲。
         * @param topology 输入顶点的图元组装方式。
         */
        static void DrawIndexed(
            const VertexArray &vertexArray,
            const PrimitiveTopology topology = PrimitiveTopology::TriangleList
        )
        {
            if (RendererAPI *api = GetRendererAPI())
                api->DrawIndexed(vertexArray, topology);
        }

        static void SetViewport(
            const uint32_t x,
            const uint32_t y,
            const uint32_t width,
            const uint32_t height
        )
        {
            if (RendererAPI *api = GetRendererAPI())
            {
                api->SetViewport(
                    x,
                    y,
                    width,
                    height
                );
            }
        }

    private:
        /**
         * @return 已初始化的RendererAPI；未初始化时记录错误并返回nullptr。
         */
        static RendererAPI *GetRendererAPI() noexcept;

        static Scope<RendererAPI> s_RendererAPI;
    };
}
