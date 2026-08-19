//
// Created by chenlong on 2026/8/12.
//

#pragma once
#include "Renderer/RendererAPI.h"

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

        void DrawIndexed(const VertexArray &vertexArray) override;
    };
}
