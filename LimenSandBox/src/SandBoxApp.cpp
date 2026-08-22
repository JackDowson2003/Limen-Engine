#include "Limen.h"

#include "Example3DLayer.h"
#include "SandBox2D.h"


//ExampleLayer2D


namespace
{
    class SandBoxApp : public Limen::Application
    {
    public:
        SandBoxApp()
            : Application(
                false,
                Limen::RendererAPI::API::OPENGL
            )
        {
            PushLayer(new SandBox2D());
            // 暂时只运行3D测试，先隔离验证透视和深度。
            // PushLayer(new SandBox::Example3DLayer());
        }

        ~SandBoxApp() override = default;
    };
}

//不实现是无法run的
Limen::Application *Limen::CreateApplication()
{
    return new SandBoxApp();
}
