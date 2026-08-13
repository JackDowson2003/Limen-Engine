#include "Limen.h"
#include <imgui.h>


namespace
{
    class ExampleLayer : public Limen::Layer
    {
    public:
        ExampleLayer()
            : Layer("example layer"),
              m_Camera(Limen::OrthoGraphicCamera(-2.f, 2.f, -1.f, 1.f))
        {
            constexpr float vertices[] = {
                -0.5f, -0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
                0.5f, -0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
                0.5f, 0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
            };

            //Vertex Array
            m_VertexArray.reset(Limen::VertexArray::Create());
            //Vertex Buffer
            m_VertexBuffer.reset(Limen::VertexBuffer::Create(vertices, sizeof(vertices)));
            const Limen::BufferLayout layout
            {
                {Limen::ShaderDataType::Float3, "a_Position"},
                {Limen::ShaderDataType::Float4, "a_Color"},
            };
            m_VertexBuffer->SetLayout(layout);
            m_VertexArray->AddVertexBuffer(m_VertexBuffer);

            //Index Buffer
            constexpr uint32_t indices[] = {0, 1, 2};
            m_IndexBuffer.reset(Limen::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
            m_VertexArray->SetIndexBuffer(m_IndexBuffer);
            //====================
            m_SquareVAO.reset(Limen::VertexArray::Create());
            constexpr float SquareVertices[] = {
                0.5f, -0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
                0.7f, -0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
                0.7f, 0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
                0.5f, 0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
            };

            m_VertexBuffer.reset(Limen::VertexBuffer::Create(SquareVertices, sizeof(SquareVertices)));
            m_VertexBuffer->SetLayout(layout);

            m_SquareVAO->AddVertexBuffer(m_VertexBuffer);

            constexpr uint32_t SquareIndices[] = {
                0, 1, 2,
                2, 3, 0
            };
            m_IndexBuffer.reset(Limen::IndexBuffer::Create(SquareIndices, sizeof(SquareIndices) / sizeof(uint32_t)));
            m_SquareVAO->SetIndexBuffer(m_IndexBuffer);


            //Shader
            const std::string vertexSource = R"(
            #version 410 core

            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec4 a_Color;

            uniform mat4 u_ViewProjection;

            out vec4 v_Color;
            out vec3 v_Position;

            void main()
            {
                v_Color = a_Color;
                v_Position = a_Position;
                // 裁剪空间坐标的 w 必须为 1，三角形才能正常进行透视除法。proj * view * P_local 暂时没有model，view = (T * R) -1
                gl_Position = u_ViewProjection * vec4(a_Position, 1.0);

            }
        )";

            const std::string fragmentSource = R"(
             #version 410 core

             layout(location = 0) out vec4 color;




             in vec3 v_Position;
             in vec4 v_Color;


             void main()
             {
                 // 亮蓝色，四个分量依次是 RGBA。
                 color = vec4(v_Position* 0.5 + 0.5, 1.0);
                 //color = v_Color;
             }
         )";

            m_Shader = Limen::Shader::Create(vertexSource, fragmentSource);
            m_VertexArray->UnBind();
            m_SquareVAO->UnBind();
            m_Camera.SetPosition({-.2f, -.2f, 0.f});
            m_Camera.SetRotation(0.f);
        }

        void OnUpdate( Limen::DeltaTime &dt) override
        {
            dt = std::min(dt.GetSeconds(), 0.05f);
            // 深色背景能清楚显示亮蓝色三角形。
            Limen::RendererCommand::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});
            Limen::RendererCommand::Clear();
            constexpr float moveSpeed = 1.0f;
            auto &position = m_Camera.GetPosition();
            if (Limen::Input::IsKeyPressed(Limen::KeyCode::Space))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                constexpr float rotationSpeed = 360.0f;
                m_Camera.SetRotation(m_Camera.GetRotation() - dt * rotationSpeed);
            }
            if (Limen::Input::IsKeyPressed(Limen::KeyCode::W))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());

                m_Camera.SetPosition({position.x , position.y- dt * moveSpeed, position.z});
            }
            else if (Limen::Input::IsKeyPressed(Limen::KeyCode::S))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                m_Camera.SetPosition({position.x, position.y + dt * moveSpeed, position.z});
            }
            else if (Limen::Input::IsKeyPressed(Limen::KeyCode::A))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                m_Camera.SetPosition({position.x + dt * moveSpeed, position.y , position.z});
            }else if (Limen::Input::IsKeyPressed(Limen::KeyCode::D))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                m_Camera.SetPosition({position.x - dt * moveSpeed, position.y , position.z});
            }

            Limen::Renderer::BeginScene(m_Camera);
            Limen::Renderer::Submit(*m_Shader, *m_VertexArray);
            m_Shader->UploadUniformMat4("u_ViewProjection", m_Camera.GetViewProjectionMatrix());
            Limen::Renderer::Submit(*m_Shader, *m_SquareVAO);
            Limen::Renderer::EndScene();
        }

        void OnEvent(Limen::Event &e) override
        {
            // Limen::EventDispatcher dispatcher(e);
            // dispatcher.Dispatch<>()
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

    private:
        Limen::OrthoGraphicCamera m_Camera;
        std::shared_ptr<Limen::VertexArray> m_SquareVAO;
        std::shared_ptr<Limen::VertexArray> m_VertexArray;
        std::shared_ptr<Limen::Shader> m_Shader;
        std::shared_ptr<Limen::VertexBuffer> m_VertexBuffer;
        std::shared_ptr<Limen::IndexBuffer> m_IndexBuffer;
    };
}


namespace
{
    class SandBoxApp : public Limen::Application
    {
    public:
        SandBoxApp()
            : Application(false)
        {
            PushOverlay(new ExampleLayer());
        }

        ~SandBoxApp() override = default;
    };
}

//不实现是无法run的
Limen::Application *Limen::CreateApplication()
{
    return new SandBoxApp();
}
