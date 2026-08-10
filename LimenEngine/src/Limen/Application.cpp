//
// Created by chenlong on 2026/8/5.
//
#include <glad/gl.h>

#include "Application.h"

#include "Events/ApplicationEvent.h"
#include "ImGUI/ImGUILayer.h"
#include "Log.h"

namespace Limen
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        LM_CORE_ASSERT(!s_Instance, "Application already initialized!");
        s_Instance = this;
        m_Window = std::unique_ptr<Window>(Window::Create());
        //发生Events的时候，就调用这个匿名函数,也就是OnEvent()
        m_Window->SetEventCallback([this](Event& e)
        {
            OnEvent(e);
        });

#if defined(LIMEN_PLATFORM_LINUX) || defined(LIMEN_PLATFORM_MACOS)
        // 编辑器 UI 由 Application 统一管理，客户端只需实现 OnImGuiRender()。
        m_ImGUILayer = new ImGUILayer();
        PushOverlay(m_ImGUILayer);
#endif
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e){
           return OnWindowClose(e);
        });
        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            if (e.IsHandled())
                break;

            (*it)->OnEvent(e);
        }
        LM_CORE_TRACE("{0}",e.ToString());
    }

    bool Application::OnWindowClose(WindowCloseEvent &e)
    {
        m_Running = false;
        return true;
    }


    Application::~Application() = default;

    void Application::Run()
    {
        while (m_Running)
        {
            glClearColor(1.F, 1.F, 0.1F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);
            // Application.cpp
            for (Layer* layer : m_LayerStack)
                layer->OnUpdate();

#if defined(LIMEN_PLATFORM_LINUX) || defined(LIMEN_PLATFORM_MACOS)
            LM_CORE_ASSERT(m_ImGUILayer, "ImGui layer was not initialized");
            m_ImGUILayer->Begin();

            for (Layer*& layer : m_LayerStack)
                layer->OnImGuiRender();

            m_ImGUILayer->End();
#endif

            m_Window->OnUpdate();
        }
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
