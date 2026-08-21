#include "Limen.h"
#include <imgui.h>
#include <glm/ext/matrix_transform.hpp>

#include "Example3DLayer.h"
#include "../../LimenEngine/src/RHI/macOS/OpenGL/OpenGLShader.h"
#include "glm/gtc/type_ptr.hpp"


namespace Limen
{
    class OpenGLShader;
}

//ExampleLayer2D
namespace
{
    class ExampleLayer : public Limen::Layer
    {
    public:
        ExampleLayer()
            : Layer("example layer"),
              m_Camera(Limen::OrthographicCamera(-2.f, 2.f, -1.f, 1.f)),
              m_Position(glm::vec3(0.0f)),
              m_Scale(glm::vec3(1.0f, 1.0f, 1.0f))
        {
            constexpr float vertices[] = {
                -0.5f, -0.5f, 0.0f,
                0.5f, -0.5f, 0.0f,
                0.5f, 0.5f, 0.0f,
            };

            //Vertex Array
            m_VertexArray.reset(Limen::VertexArray::Create());
            //Vertex Buffer
            m_VertexBuffer.reset(Limen::VertexBuffer::Create(vertices, sizeof(vertices)));
            const Limen::BufferLayout layout
            {
                {Limen::ShaderDataType::Float3, "a_Position"},
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
                // position              //texture
                -0.05f, -0.05f, 0.0f, 0.f, 0.f,
                0.05f, -0.05f, 0.0f, 1.f, 0.f,
                0.05f, 0.05f, 0.0f, 1.f, 1.f,
                -0.05f, 0.05f, 0.0f, 0.f, 1.f,
            };
            //VAO Set IBO
            m_VertexArray->SetIndexBuffer(m_IndexBuffer);

            //Layout2
            const Limen::BufferLayout layout2
            {
                {Limen::ShaderDataType::Float3, "a_Position"},
                {Limen::ShaderDataType::Float2, "a_TexCoord"},
            };

            m_SquareVBO.reset(Limen::VertexBuffer::Create(SquareVertices, sizeof(SquareVertices)));
            m_SquareVBO->SetLayout(layout2);
            m_SquareVAO->AddVertexBuffer(m_SquareVBO);

            constexpr uint32_t squareIndices[] = {
                0, 1, 2,
                2, 3, 0
            };
            m_IndexBuffer.reset(Limen::IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
            m_SquareVAO->SetIndexBuffer(m_IndexBuffer);

            /**
             * @brief 从两个GLSL文件创建2D纹理Shader Program。
             *
             * 路径省略assets/shaders前缀，由Shader底层统一补全资源目录。
             * Shader逻辑名称自动从Texture2D.vert提取为Texture2D。
             */
            // m_TextureShader = Limen::Shader::CreateFromFiles(
            //     "OpenGL/Example2D/Texture2D.vert",
            //     "OpenGL/Example2D/Texture2D.frag"
            // );

            m_TextureShader = m_ShaderLib->Load("OpenGL/Example2D/Texture2D.vert",
                "OpenGL/Example2D/Texture2D.frag");

            LM_CORE_ASSERT(
                m_TextureShader,
                "Failed to create Example2D Texture2D shader"
            );

            m_VertexArray->UnBind();
            /**
             * 从外部GLSL文件创建2D纯色Shader。
             */
            m_FlatColorShader = m_ShaderLib->Load(
                "OpenGL/Example2D/FlatColor.vert",
                "OpenGL/Example2D/FlatColor.frag"
            );

            LM_CORE_ASSERT(
                m_FlatColorShader,
                "Failed to create Example2D FlatColor shader"
            );

            m_SquareVAO->UnBind();
            m_Camera.SetPosition({-.2f, -.2f, 0.f});
            m_Camera.SetRotation(0.f);

            m_Texture = Limen::Texture2D::Create("assets/textures/checkerboard.png");
            m_LogoTexture = Limen::Texture2D::Create("assets/textures/cherno.png");

            std::dynamic_pointer_cast<Limen::OpenGLShader>(m_TextureShader)->Bind();
            std::dynamic_pointer_cast<Limen::OpenGLShader>(m_TextureShader)->UploadUniformInt("u_Texture", 0);
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

            //必须先缩放 再平移  cuz T*S != S*T.
            //Excepted behaviour: scale the object locally, then translate it to target world position.
            const auto scale = glm::scale(glm::mat4(1.0f), m_Scale);
            m_FlatColorShader->Bind();
            // m_FlatColorShader->Bind();
            std::dynamic_pointer_cast<Limen::OpenGLShader>(m_FlatColorShader)->UploadUniformFloat3(
                "u_Color", m_SquareColor);
            for (int y = 0; y < 20; y++)
            {
                for (int x = 0; x < 20; x++)
                {
                    m_Position = {0.11f * static_cast<float>(x), static_cast<float>(y) * 0.11f, 0.f};
                    const auto transform = glm::translate(glm::mat4(1.0f), m_Position);

                    Limen::Renderer::Submit(m_FlatColorShader, *m_SquareVAO, transform * scale);
                }
            }
            // std::dynamic_pointer_cast<Limen::OpenGLShader>(m_Shader)->UploadUniformMat4(
            //     "u_ViewProjection", m_Camera.GetViewProjectionMatrix());
            std::dynamic_pointer_cast<Limen::OpenGLShader>(m_TextureShader)->Bind();

            // m_Texture->Bind(0);
            // Limen::Renderer::Submit(m_TextureShader, *m_SquareVAO,glm::scale(glm::mat4(1.0f), glm::vec3(13.f)));
            //
            // m_LogoTexture->Bind(1);
            // Limen::Renderer::Submit(m_TextureShader, *m_SquareVAO,glm::scale(glm::mat4(1.0f), glm::vec3(13.f)));

            m_Texture->Bind();

            Limen::Renderer::Submit(
                m_TextureShader,
                *m_SquareVAO,
                glm::scale(glm::mat4(1.0f), glm::vec3(13.0f))
            );
            m_LogoTexture->Bind();
            Limen::Renderer::Submit(
                m_TextureShader,
                *m_SquareVAO,
                glm::scale(glm::mat4(1.0f), glm::vec3(13.0f))
            );

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
        Limen::OrthographicCamera m_Camera;

        // Ref默认是空指针；创建实际ShaderLibrary后才能调用Load()。
        Limen::Ref<Limen::ShaderLibrary> m_ShaderLib = Limen::CreateRef<Limen::ShaderLibrary>();

        Limen::Scope<Limen::VertexArray> m_SquareVAO;
        Limen::Scope<Limen::VertexArray> m_VertexArray;
        Limen::Ref<Limen::Shader> m_FlatColorShader, m_TextureShader;

        Limen::Ref<Limen::Texture2D> m_Texture, m_LogoTexture;

        Limen::Ref<Limen::VertexBuffer> m_VertexBuffer;
        Limen::Ref<Limen::VertexBuffer> m_SquareVBO;
        Limen::Ref<Limen::IndexBuffer> m_IndexBuffer;

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
            : Application(
                false,
                Limen::RendererAPI::API::OPENGL
            )
        {
            // PushLayer(new ExampleLayer());
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
