#include "Limen.h"
#include <imgui.h>
#include <glm/ext/matrix_transform.hpp>

#include "../../LimenEngine/src/Platform/Mac/OpenGL/OpenGLShader.h"
#include "glm/gtc/type_ptr.hpp"


namespace
{
    class ExampleLayer : public Limen::Layer
    {
    public:
        ExampleLayer()
            : Layer("example layer"),
              m_Camera(Limen::OrthoGraphicCamera(-2.f, 2.f, -1.f, 1.f)),
              m_Position(glm::vec3(0.0f)),
              m_Scale(glm::vec3(1.0f, 1.0f, 1.0f))
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
                // position                // color
                -0.05f, -0.05f, 0.0f, 1.f, 0.f, 0.f, 1.f,
                0.05f, -0.05f, 0.0f, 1.f, 0.f, 0.f, 1.f,
                0.05f, 0.05f, 0.0f, 1.f, 0.f, 0.f, 1.f,
                -0.05f, 0.05f, 0.0f, 1.f, 0.f, 0.f, 1.f,
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
            uniform mat4 u_Transform;

            out vec4 v_Color;
            out vec3 v_Position;

            void main()
            {
                v_Color = a_Color;
                v_Position = a_Position;
                // 裁剪空间坐标的 w 必须为 1，三角形才能正常进行透视除法。proj * view * P_local 暂时没有model，view = (T * R) -1
                gl_Position = u_ViewProjection *u_Transform* vec4(a_Position, 1.0);

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

            const std::string blueVertexSource = R"(
            #version 410 core

            layout(location = 0) in vec3 a_Position;

            uniform mat4 u_ViewProjection;
            uniform mat4 u_Transform;

            out vec3 v_Position;

            void main()
            {
                v_Position = a_Position;
                // 裁剪空间坐标的 w 必须为 1，三角形才能正常进行透视除法。proj * view * P_local 暂时没有model，view = (T * R) -1
                gl_Position = u_ViewProjection *u_Transform* vec4(a_Position, 1.0);

            }
        )";

            const std::string flatShaderFragmentSource = R"(
             #version 410 core

             layout(location = 0) out vec4 color;

             in vec3 v_Position;

             uniform vec3 u_Color;



             void main()
             {
                 // 亮蓝色，四个分量依次是 RGBA。
                 color = vec4( u_Color,1.0);
             }
         )";

            m_Shader = Limen::Shader::Create(vertexSource, fragmentSource);
            m_VertexArray->UnBind();
            m_BlueShader = Limen::Shader::Create(blueVertexSource, flatShaderFragmentSource);

            m_SquareVAO->UnBind();
            m_Camera.SetPosition({-.2f, -.2f, 0.f});
            m_Camera.SetRotation(0.f);
        }


        void OnUpdate(Limen::DeltaTime &dt) override
        {
            dt = std::min(dt.GetSeconds(), 0.05f);
            // 深色背景能清楚显示亮蓝色三角形。
            Limen::RendererCommand::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});
            Limen::RendererCommand::Clear();
            auto &position = m_Camera.GetPosition();
            //region if block
            if (Limen::Input::IsKeyPressed(Limen::KeyCode::Space))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                m_Camera.SetRotation(m_Camera.GetRotation() - dt * m_RotateSpeed);
            }

            if (Limen::Input::IsKeyPressed(Limen::KeyCode::W))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());

                m_Camera.SetPosition({position.x, position.y - dt * m_MoveSpeed, position.z});
            } else if (Limen::Input::IsKeyPressed(Limen::KeyCode::S))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                m_Camera.SetPosition({position.x, position.y + dt * m_MoveSpeed, position.z});
            } else if (Limen::Input::IsKeyPressed(Limen::KeyCode::A))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                m_Camera.SetPosition({position.x + dt * m_MoveSpeed, position.y, position.z});
            } else if (Limen::Input::IsKeyPressed(Limen::KeyCode::D))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                m_Camera.SetPosition({position.x - dt * m_MoveSpeed, position.y, position.z});
            }

            if (Limen::Input::IsKeyPressed(Limen::KeyCode::Up))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());

                m_Position.y += dt * m_MoveSpeed;
            } else if (Limen::Input::IsKeyPressed(Limen::KeyCode::Down))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                m_Position.y -= dt * m_MoveSpeed;
            } else if (Limen::Input::IsKeyPressed(Limen::KeyCode::Left))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                m_Position.x -= dt * m_MoveSpeed;
            } else if (Limen::Input::IsKeyPressed(Limen::KeyCode::Right))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                m_Position.x += dt * m_MoveSpeed;
            }


            if (Limen::Input::IsKeyPressed(Limen::KeyCode::KeypadAdd))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());

                m_Scale += dt * m_ScaleSpeed;
            } else if (Limen::Input::IsKeyPressed(Limen::KeyCode::KeypadSubtract))
            {
                LM_TRACE("Delta Time : {0}ms", dt.GetMilliseconds());
                m_Scale -= dt * m_ScaleSpeed;
            }

            //endregion
            Limen::Renderer::BeginScene(m_Camera);
            std::dynamic_pointer_cast<Limen::OpenGLShader>(m_Shader)->UploadUniformMat4(
                "u_ViewProjection", m_Camera.GetViewProjectionMatrix());
            Limen::Renderer::Submit(m_Shader, *m_VertexArray);
            //必须先缩放 再平移  cuz T*S != S*T.
            //Excepted behaviour: scale the object locally, then translate it to target world position.
            const auto scale = glm::scale(glm::mat4(1.0f), m_Scale);

            // Limen::Material *material = new Limen::Material(m_BlueShader);
            // Limen::MaterialInstanceRef *mi = new Limen::MaterialInstanceRef(material);
            // const Limen::Texture2D *texture = new Limen::Texture2D("");
            //
            // mi->SetVal("u_Color", redColor);
            // mi->SetTexture("u_AlbedoMap", texture);
            //
            // squareMesh->SetMaterial(mi);

            for (int y = 0; y < 20; y++)
            {
                for (int x = 0; x < 20; x++)
                {
                    m_Position = {0.11f * static_cast<float>(x), static_cast<float>(y) * 0.11f, 0.f};
                    const auto transform = glm::translate(glm::mat4(1.0f), m_Position);
                    m_BlueShader->Bind();
                    std::dynamic_pointer_cast<Limen::OpenGLShader>(m_BlueShader)->UploadUniformFloat3(
                                    "u_Color", m_SquareColor);
                    Limen::Renderer::Submit(m_BlueShader, *m_SquareVAO, transform * scale);
                }
            }
            Limen::Renderer::EndScene();
        }

        void OnEvent(Limen::Event &e) override
        {
            // Limen::EventDispatcher dispatcher(e);
            // dispatcher.Dispatch<>()
        }

        void OnImGuiRender() override
        {
            ImGui::Begin("Settings");
            ImGui::ColorEdit3("Square Color", glm::value_ptr(m_SquareColor));


            ImGui::End();
        }

    private:
        Limen::OrthoGraphicCamera m_Camera;

        std::shared_ptr<Limen::VertexArray> m_SquareVAO;
        std::shared_ptr<Limen::VertexArray> m_VertexArray;
        std::shared_ptr<Limen::Shader> m_Shader;
        std::shared_ptr<Limen::Shader> m_BlueShader;

        std::shared_ptr<Limen::VertexBuffer> m_VertexBuffer;
        std::shared_ptr<Limen::IndexBuffer> m_IndexBuffer;

        float m_MoveSpeed = 1.0f;
        float m_RotateSpeed = 360.0f;
        float m_ScaleSpeed = 1.0f;


        glm::vec3 m_Position;
        glm::vec3 m_Scale;
        glm::vec3 m_SquareColor{0.2f, 0.3f, 0.8f};
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
            PushLayer(new ExampleLayer());
        }

        ~SandBoxApp() override = default;
    };
}

//不实现是无法run的
Limen::Application *Limen::CreateApplication()
{
    return new SandBoxApp();
}
