//
// Created by chenlong on 2026/8/5.
//

#pragma once
#include "Core.h"
#include "Window.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"
#include "Renderer/OrthoGraphicCamera.h"

namespace Limen
{
    class ImGUILayer;

    class LIMEN_API Application //静态链接不需要写__declspec(dllexport)
    {
    public:

        Application(bool isVSYNC = true);

        virtual ~Application(); //交给sandbox去实现

        void Run();
        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);

        void OnEvent(Event& e);

        [[nodiscard]] inline Window& GetWindow() const { return *m_Window; }

        inline static  Application& GetApp() { return *s_Instance; }

    private:
        bool OnWindowClose(WindowCloseEvent& e);

        Scope<Window> m_Window;
        ImGUILayer* m_ImGUILayer = nullptr;
        // LayerStack 负责销毁所有 Layer；这里仅保存 ImGui Layer 的非拥有引用。
        LayerStack m_LayerStack;
        bool m_Running = true;

        double m_LastFrameTime = 0.0f;

    private:
        static Application *s_Instance;

    };

    //To be defined in CLIENT
    Application *CreateApplication();
}
