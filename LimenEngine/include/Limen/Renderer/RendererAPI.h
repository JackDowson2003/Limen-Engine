//
// Created by chenlong on 2026/8/12.
//

#pragma once
#include "VertexArray.h"
#include "glm/glm.hpp"

namespace Limen
{
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

    public:
        virtual void Clear() = 0;

        virtual void SetClearColor(const glm::vec4 &color) = 0;

        virtual void Init() = 0;

        virtual void DrawIndexed(const VertexArray &vertexArray) = 0;

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

        /**
         *
         * @return API type
         */
        static API GetAPI()
        {
            return API::OPENGL;
        }

    private:
        static API s_API;
    };
}
