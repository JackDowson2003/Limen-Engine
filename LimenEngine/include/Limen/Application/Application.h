//
// Created by chenlong on 2026/8/5.
//

#pragma once
#include "Limen/Core/Core.h"
#include "Limen/Application/Window.h"
#include "Limen/Events/ApplicationEvent.h"
#include "Limen/Application/LayerStack.h"
#include "Limen/RHI/RendererAPI.h"

namespace Limen
{
    class ImGUILayer;

    class LIMEN_API Application
    {
    public:

        /**
         * @brief 创建应用并在窗口创建前选择图形API。
         *
         * @param isVSYNC 是否启用垂直同步。
         * @param rendererAPI 本次运行使用的图形API。macOS可选择OpenGL或Metal；
         *                    当前阶段仅OpenGL后端已经实现。
         */
        explicit Application(
            bool isVSYNC = true,
            RendererAPI::API rendererAPI = RendererAPI::API::OPENGL
        );

        virtual ~Application();

        void Run();
        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);

        void OnEvent(Event& e);

        /** 最近一次 BeginFrame～EndFrame 的 CPU 工作耗时，单位毫秒；-1 表示尚无结果。 */
        [[nodiscard]] double GetLastCPUFrameWorkMilliseconds() const noexcept
        {
            return m_LastCPUFrameWorkMilliseconds;
        }

        [[nodiscard]] Window& GetWindow() const { return *m_Window; }

        static  Application& GetApp() { return *s_Instance; }

    private:
        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

        Scope<Window> m_Window;
        ImGUILayer* m_ImGUILayer = nullptr;
        // LayerStack 拥有并销毁所有 Layer；此处只是指向其中 ImGui Layer 的观察指针。
        LayerStack m_LayerStack;
        bool m_Running = true;
        bool m_Minimized = false;

        double m_LastFrameTime = 0.0f;

        // 最近一次 BeginFrame～EndFrame 的 CPU 工作耗时，单位毫秒；-1 表示尚未测量。
        double m_LastCPUFrameWorkMilliseconds = -1.0;

        static Application *s_Instance;

    };

    // 由客户端实现，EntryPoint 使用它创建具体的 Application。
    Application *CreateApplication();
}
