//
// Created by chenlong on 2026/8/5.
//

#pragma once
#include "Core.h"
#include "Window.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"
namespace Limen
{
    class LIMEN_API Application //静态链接不需要写__declspec(dllexport)
    {
    public:

        Application();

        virtual ~Application(); //交给sandbox去实现

        void Run();
        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);

        void OnEvent(Event& e);

        inline Window& GetWindow() { return *m_Window; }

        inline static  Application& GetApp() { return *s_Instance; }

    private:
        bool OnWindowClose(WindowCloseEvent& e);

        std::unique_ptr<Window> m_Window;
        LayerStack m_LayerStack;
        bool m_Running = true;

    private:
        static Application *s_Instance;

    };

    //To be defined in CLIENT
    Application *CreateApplication();
}
