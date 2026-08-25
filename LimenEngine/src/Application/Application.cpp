//
// Created by chenlong on 2026/8/5.
//


#include "Limen/Application/Application.h"
#include "Limen/Events/ApplicationEvent.h"
#include "Editor/ImGui/ImGUILayer.h"
#include "Limen/Core/Log.h"
#include "GLFW/glfw3.h"
#include "Limen/Renderer/Renderer.h"

namespace Limen
{
    Application *Application::s_Instance = nullptr;

    Application::Application(
        const bool isVSYNC,
        const RendererAPI::API rendererAPI
    )
    {
        LM_CORE_ASSERT(!s_Instance, "Application already initialized!");
        s_Instance = this;

        // 图形 API 必须先于 Window/GraphicsContext 确定；同一个 MacWindow
        // 以后可以根据该选择承载 OpenGLContext 或 MetalContext。
        RendererAPI::SetAPI(rendererAPI);

        m_Window = Scope<Window>(Window::Create());
        m_Window->SetVSync(isVSYNC);
        // Window 将原生事件转换为引擎 Event 后，通过该回调交给 Application。
        m_Window->SetEventCallback([this](Event &e)
        {
            OnEvent(e);
        });

        Renderer::Init();

#if defined(LIMEN_PLATFORM_LINUX) || defined(LIMEN_PLATFORM_MACOS)
        // 编辑器 UI 由 Application 统一管理，客户端只需实现 OnImGuiRender()。
        m_ImGUILayer = new ImGUILayer();
        PushOverlay(m_ImGUILayer);
#endif
    }


    Application::~Application()
    {
        // Layer 中的 GPU 资源必须先于 Renderer 后端与 Window/Context 释放。
        m_LayerStack.Clear();
        m_ImGUILayer = nullptr;

        Renderer::Shutdown();

        // 最后销毁图形 Context 与原生窗口。
        m_Window.reset();
        s_Instance = nullptr;
    }

    void Application::Run()
    {
        m_LastFrameTime = glfwGetTime();
        while (m_Running)
        {
            // 帧首获取最新的窗口、键盘和鼠标事件。
            m_Window->PollEvents();

            if (!m_Running)
                break;

            const double time = glfwGetTime();
            DeltaTime deltaTime = time - m_LastFrameTime;
            m_LastFrameTime = time;
            if (!m_Minimized)
            {
                for (Layer *layer: m_LayerStack)
                    layer->OnUpdate(deltaTime); //更新数据

                /*
                 * 所有离屏场景渲染结束后，Example3DLayer 已经解绑场景 FBO，
                 * 当前渲染目标重新是窗口默认 Framebuffer。
                 *
                 * 清理上一帧窗口内容，随后 ImGui 会重新绘制编辑器界面。
                 */
                RendererCommand::SetClearColor(
                    {0.04f, 0.04f, 0.04f, 1.0f}
                );

                RendererCommand::Clear();
            }

#if defined(LIMEN_PLATFORM_LINUX) || defined(LIMEN_PLATFORM_MACOS)
            LM_CORE_ASSERT(m_ImGUILayer, "ImGui layer was not initialized");
            m_ImGUILayer->Begin();
            for (Layer *&layer: m_LayerStack)
                layer->OnImGuiRender(); //设置好imgui的数据
            m_ImGUILayer->End();
#endif

            // 所有场景与 ImGui 命令完成后再交换窗口缓冲。
            m_Window->Present(); //render
        }
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent &e)
        {
            return OnWindowClose(e);
        });
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent &e)
        {
            return OnWindowResize(e);
        });
        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            if (e.IsHandled())
                break;

            (*it)->OnEvent(e);
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent &e)
    {
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent &e)
    {
        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            m_Minimized = true;
            return false;
        }
        m_Minimized = false;
        Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
        return false;
    }

    void Application::PushLayer(Layer *layer)
    {
        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer *layer)
    {
        m_LayerStack.PushOverlay(layer);
        layer->OnAttach();
    }
}
