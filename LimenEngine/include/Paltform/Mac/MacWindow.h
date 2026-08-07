//
// Created by chenlong on 2026/8/7.
//
#pragma once

#include "Window.h"
#include <GLFW/glfw3.h>


namespace Limen
{
    class MacWindow : public Window
    {
    public:
        MacWindow(const WindowProps &props);

        ~MacWindow() override;

        void OnUpdate() override;

        inline unsigned int GetWidth() const override { return m_Data.Width; }
        inline unsigned int GetHeight() const override { return m_Data.Height; }
        void SetEventCallback(const EventCallbackFn &callback) override { m_Data.EventCallBack = callback; }

        void SetVSync(bool enabled) override;

        [[nodiscard]] bool IsVSync() const override;

    private:
        virtual void Init(const WindowProps& props);
        virtual void Shutdown();

    private:
        GLFWwindow *m_Window = nullptr;

        struct WindowData
        {
            std::string Title;
            unsigned int Width, Height;
            bool VSync;
            EventCallbackFn EventCallBack;
        };

        WindowData m_Data;
    };
}
