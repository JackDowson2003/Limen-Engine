//
// Created by chenlong on 2026/8/12.
//

#pragma once

#include <cstdint>

#include "Limen/RHI/RenderState.h"
#include "Limen/RHI/VertexArray.h"
#include "glm/glm.hpp"

namespace Limen
{
    /**
     * @brief 后端无关的即时渲染命令接口。
     *
     * RendererCommand 持有一个具体实现，并把 Clear、Viewport 和 Draw
     * 等命令转发到当前图形 API。
     */
    class LIMEN_API RendererAPI
    {
    public:
        virtual ~RendererAPI() = default;

        enum class API
        {
            NONE = 0,
            OPENGL = 1,
            VULKAN = 2,
            DIRECT12 = 3,
            DIRECT11 = 4,
            METAL = 5,
        };

        virtual void Clear() = 0;

        virtual void SetClearColor(const glm::vec4 &color) = 0;

        virtual void Init() = 0;

        /**
         * @brief 使用索引缓冲绘制顶点。
         *
         * @param vertexArray
         * 包含顶点缓冲、索引缓冲和顶点布局的VertexArray。
         *
         * @param topology
         * GPU将索引指定的顶点组装成三角形、线或点的方式。
         */
        virtual void DrawIndexed(
            const VertexArray& vertexArray,
            PrimitiveTopology topology
        ) = 0;

        virtual void SetViewport(
            uint32_t x,
            uint32_t y,
            uint32_t width,
            uint32_t height
        ) = 0;


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
        virtual void SetDepthTest(bool enabled) = 0;

        /** @return 当前进程选择的图形 API。 */
        static API GetAPI() noexcept
        {
            return s_API;
        }

        /**
         * @brief 选择本次运行使用的图形 API。
         *
         * 必须在Renderer::Init()之前调用；Renderer初始化后不允许切换后端。
         */
        static void SetAPI(API api) noexcept
        {
            s_API = api;
        }

    private:
        static API s_API;
    };
}
