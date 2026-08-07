//
// Created by chenlong on 2026/8/5.
//
#include "Application.h"

#include "Events/ApplicationEvent.h"
#include "Log.h"

#include <GLFW/glfw3.h>

namespace Limen
{
    Application::Application()
    {
        m_Window = std::unique_ptr<Window>(Window::Create());
        //发生Events的时候，就调用这个匿名函数,也就是OnEvent()
        m_Window->SetEventCallback([this](Event& e)
        {
            OnEvent(e);
        });
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e){
           return OnWindowClose(e);
        });
        for (auto*& layer: m_LayerStack)
        {
            layer->OnEvent(e);
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
        WindowResizeEvent e(1280, 720);
        while (m_Running)
        {
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
        m_LayerStack.PopOverlay(layer);
        layer->OnAttach();
    }
}
