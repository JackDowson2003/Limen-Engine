#pragma once

#include "Window.h"

#include <GLFW/glfw3.h>

#include "Limen/Renderer/GraphicsContext.h"

namespace Limen
{
    // macOS + GLFW 的窗口实现；不属于引擎对外 API。
    class MacWindow final : public Window
    {
    public:
        explicit MacWindow(const WindowProps &props);

        ~MacWindow() override;

        void OnUpdate() override;

        [[nodiscard]] inline void *GetNativeWindow() const override { return m_Window; }
        [[nodiscard]] inline unsigned int GetWidth() const override { return m_Data.Width; }
        [[nodiscard]] inline unsigned int GetHeight() const override { return m_Data.Height; }

        inline void SetEventCallback(const EventCallbackFn &callback) override { m_Data.EventCallBack = callback; }

        inline void SetVSync(bool enabled) override;

        [[nodiscard]] bool IsVSync() const override;

    private:
        void Init(const WindowProps &props);

        void Shutdown() const;

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
        GraphicsContext* m_Context;
    };
}
