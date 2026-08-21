#pragma once

#include "Limen/Core/Core.h"
#include "Limen/Application/Window.h"

#include <GLFW/glfw3.h>

#include "RHI/Common/GraphicsContext.h"

namespace Limen
{
    // macOS + GLFW 的窗口实现；不属于引擎对外 API。
    class MacWindow final : public Window
    {
    public:
        explicit MacWindow(const WindowProps &props);

        ~MacWindow() override;

        void PollEvents() override;

        void Present() override;

        [[nodiscard]] inline void *GetNativeWindow() const override { return m_Window; }
        [[nodiscard]] inline unsigned int GetWidth() const override { return m_Data.Width; }
        [[nodiscard]] inline unsigned int GetHeight() const override { return m_Data.Height; }

        inline void SetEventCallback(const EventCallbackFn &callback) override { m_Data.EventCallBack = callback; }

        inline void SetVSync(bool enabled) override;

        [[nodiscard]] bool IsVSync() const override;

    private:
        void Init(const WindowProps &props);

        /**
         * @brief 按照Context、Native Window、GLFW的顺序释放窗口系统资源。
         *
         * 函数可以安全地重复调用；已经释放的对象会被跳过。
         */
        void Shutdown();

        GLFWwindow *m_Window = nullptr;

        struct WindowData
        {
            std::string Title;
            unsigned int Width;
            unsigned int Height;
            bool VSync = false;
            EventCallbackFn EventCallBack;
        };

        WindowData m_Data;

    private:
        /**
         * @brief 当前窗口使用的图形上下文包装对象。
         *
         * MacWindow唯一拥有该对象；GraphicsContext只借用m_Window句柄，
         * 不负责销毁GLFWwindow。
         */
        Scope<GraphicsContext> m_Context;
    };
}
