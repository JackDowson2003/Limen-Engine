#include "Limen.h"

#include "Example3DLayer.h"
#include "Renderer2DTestLayer.h"
#include "SandBox2D.h"


namespace
{
    class SandBoxApp : public Limen::Application
    {
    public:
        SandBoxApp(int argc, char **argv)
            : Application(
                false,
                Limen::RendererAPI::API::OPENGL,
                true // Sandbox 显式启用 ImGui 调试层。
            )
        {
            (void)argc;
            (void)argv;
            // 当前只运行 3D 测试层，便于隔离验证透视、深度与 Pipeline。
            PushLayer(new SandBox::Example3DLayer());
            PushLayer(new SandBox::Renderer2DTestLayer());
        }

        ~SandBoxApp() override = default;
    };
}

// EntryPoint.h 调用该工厂创建客户端 Application。
Limen::Application *Limen::CreateApplication(int argc, char **argv)
{
    return new SandBoxApp(argc,argv);
}
