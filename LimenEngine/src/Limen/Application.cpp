//
// Created by chenlong on 2026/8/5.
//


#include "Application.h"
#include "Events/ApplicationEvent.h"
#include "ImGUI/ImGUILayer.h"
#include "Log.h"
#include "GLFW/glfw3.h"
#include "Renderer/Renderer.h"

namespace Limen
{
    Application *Application::s_Instance = nullptr;

    Application::Application(const bool isVSYNC)
    {
        LM_CORE_ASSERT(!s_Instance, "Application already initialized!");
        s_Instance = this;
        m_Window = Scope<Window>(Window::Create());
        m_Window->SetVSync(isVSYNC);
        //发生Events的时候，就调用这个匿名函数,也就是OnEvent()
        m_Window->SetEventCallback([this](Event &e)
        {
            OnEvent(e);
        });

        //初始化参数
        Renderer::Init();

#if defined(LIMEN_PLATFORM_LINUX) || defined(LIMEN_PLATFORM_MACOS)
        // 编辑器 UI 由 Application 统一管理，客户端只需实现 OnImGuiRender()。
        m_ImGUILayer = new ImGUILayer();
        PushOverlay(m_ImGUILayer);
#endif
    }


    Application::~Application()
    {
        // Layer中的GPU资源必须先于Renderer后端和Window/Context释放。
        m_LayerStack.Clear();
        m_ImGUILayer = nullptr;

        Renderer::Shutdown();

        // 最后销毁Context和Native Window。
        m_Window.reset();
        s_Instance = nullptr;
    }

    void Application::Run()
    {
        m_LastFrameTime = glfwGetTime();
        while (m_Running)
        {
            // 帧首获取最新窗口、键盘和鼠标事件。
            m_Window->PollEvents();

            if (!m_Running)
                break;

            const double time = glfwGetTime();
            DeltaTime deltaTime = time-m_LastFrameTime;
            m_LastFrameTime = time;
            // Application.cpp
            for (Layer *layer: m_LayerStack)
                layer->OnUpdate(deltaTime);

#if defined(LIMEN_PLATFORM_LINUX) || defined(LIMEN_PLATFORM_MACOS)
            LM_CORE_ASSERT(m_ImGUILayer, "ImGui layer was not initialized");
            m_ImGUILayer->Begin();
            for (Layer *&layer: m_LayerStack)
                layer->OnImGuiRender();
            m_ImGUILayer->End();
#endif

            // 所有场景和ImGui绘制完成后再Present。
            m_Window->Present();
        }
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent &e)
        {
            return OnWindowClose(e);
        });
        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            if (e.IsHandled())
                break;

            (*it)->OnEvent(e);
        }
        // LM_CORE_TRACE("{0}",e.ToString());
    }

    bool Application::OnWindowClose(WindowCloseEvent &e)
    {
        m_Running = false;
        return true;
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
