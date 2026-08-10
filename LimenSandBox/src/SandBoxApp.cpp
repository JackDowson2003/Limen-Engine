#include "Limen.h"
#include <imgui.h>

class ExampleLayer : public Limen::Layer
{
public:
    ExampleLayer()
        : Layer("example layer")
    {
    }

    void OnUpdate() override
    {
        if (Limen::Input::IsKeyPressed(Limen::KeyCode::Space))
            LM_INFO("space key pressed");
    }

    void OnEvent(Limen::Event &e) override
    {
        LM_INFO("ExampleLayer::OnEvent");
    }

    void OnImGuiRender() override
    {
        static float value = 0.0f;
        static bool wasDocked = false;
        constexpr ImVec2 normalWindowSize{480.0f, 320.0f};

        ImGui::Begin("Example");

        const bool isDocked = ImGui::IsWindowDocked();
        if (wasDocked && !isDocked)
            ImGui::SetWindowSize(normalWindowSize, ImGuiCond_Always);
        wasDocked = isDocked;

        ImGui::Text("Drag this window to dock it inside the application.");
        ImGui::SliderFloat("Value", &value, 0.0f, 1.0f);
        ImGui::End();
    }
};


class SandBoxApp : public Limen::Application
{
public:
    SandBoxApp()
    {
        PushLayer(new ExampleLayer());
    }

    ~SandBoxApp() override = default;
};

//不实现是无法run的
Limen::Application *Limen::CreateApplication()
{
    return new SandBoxApp();
}
