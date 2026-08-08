#include "Limen.h"

class ExampleLayer : public Limen::Layer
{
public:
    ExampleLayer()
        :Layer("example layer")
    {}

    void OnUpdate() override
    {
        LM_INFO("ExampleLayer::OnUpdate");

    }

    void OnEvent(Limen::Event &e) override
    {
        LM_INFO("ExampleLayer::OnEvent");
    }
};


class SandBoxApp : public Limen::Application
{
public:
    SandBoxApp()
    {
        PushLayer(new ExampleLayer());
        PushOverlay(new Limen::ImGUILayer());
    }

    ~SandBoxApp()
    {
    }
};

//不实现是无法run的
Limen::Application* Limen::CreateApplication()
{
    return new SandBoxApp();
}