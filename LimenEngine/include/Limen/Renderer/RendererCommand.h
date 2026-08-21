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
            if (RendererAPI* api = GetRendererAPI())
                api->Clear();
        }

        static void SetClearColor(const glm::vec4 &color)
        {
            if (RendererAPI* api = GetRendererAPI())
                api->SetClearColor(color);
        }

        /**
         * @brief 根据RendererAPI::GetAPI()和当前平台创建并初始化后端。
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
            if (RendererAPI* api = GetRendererAPI())
                api->SetDepthTest(enabled);
        }

        static void DrawIndexed(const VertexArray &vao)
        {
            if (RendererAPI* api = GetRendererAPI())
                api->DrawIndexed(vao);
        }

    private:
        /**
         * @return 已初始化的RendererAPI；未初始化时记录错误并返回nullptr。
         */
        static RendererAPI* GetRendererAPI() noexcept;

        static Scope<RendererAPI> s_RendererAPI;
    };
}
