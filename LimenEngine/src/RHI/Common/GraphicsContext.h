//
// Created by chenlong on 2026/8/10.
//

#pragma once

#include "Limen/Core/Core.h"

namespace Limen
{
    /**
     * @brief 图形上下文
     */
    class  GraphicsContext
    {
    public:
        GraphicsContext() = default;
        virtual ~GraphicsContext() = default;

        /**
         * @brief 根据RendererAPI::GetAPI()创建图形上下文。
         *
         * @param nativeWindow 已创建的原生窗口句柄；Context只借用该句柄。
         */
        [[nodiscard]] static Scope<GraphicsContext> Create(void* nativeWindow);

        virtual void Init() = 0;

        /**
         * @brief 将渲染完成的图像呈现到窗口。
         *
         * OpenGL中对应glfwSwapBuffers；Metal中将对应提交并present drawable。
         */
        virtual void Present() = 0;

        /**
         * @brief 配置当前后端的垂直同步策略。
         */
        virtual void SetVSync(bool enabled) = 0;

        virtual void Shutdown() = 0;

    };
}
