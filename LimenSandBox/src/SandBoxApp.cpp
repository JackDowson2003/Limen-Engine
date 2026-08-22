#include "Limen.h"
#include <imgui.h>
#include <glm/ext/matrix_transform.hpp>

#include "Example3DLayer.h"
#include "../../LimenEngine/src/RHI/macOS/OpenGL/OpenGLShader.h"
#include "glm/gtc/type_ptr.hpp"


//ExampleLayer2D
namespace
{
    class ExampleLayer : public Limen::Layer
    {
    public:
        ExampleLayer()
            : Layer("example layer"),
              m_CameraController(1600.f/900.f,true),
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

            std::dynamic_pointer_cast<Limen::OpenGLShader>(m_FlatColorShader)->BindUniformBlock("MaterialData",0);

            m_SquareVAO->UnBind();

            m_CameraController.GetCamera().SetPosition(
                {-0.2f, -0.2f, 0.0f}
            );

            // std140会为vec3保留一个16字节槽，但颜色本身仍然只有RGB三个分量。
            constexpr uint32_t std140Vec3StorageSize = 4 * sizeof(float);
            m_UniformBuffer = Limen::UniformBuffer::Create(
                std140Vec3StorageSize,
                0
            );

            m_Texture = Limen::Texture2D::Create("assets/textures/checkerboard.png");
            m_LogoTexture = Limen::Texture2D::Create("assets/textures/cherno.png");

            std::dynamic_pointer_cast<Limen::OpenGLShader>(m_TextureShader)->Bind();
            std::dynamic_pointer_cast<Limen::OpenGLShader>(m_TextureShader)->UploadUniformInt("u_Texture", 0);
        }


        void OnUpdate(Limen::DeltaTime &dt) override
        {
            m_CameraController.OnUpdate(dt);
            // 深色背景能清楚显示亮蓝色三角形。
            Limen::RendererCommand::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});
            Limen::RendererCommand::Clear();
            Limen::Renderer::BeginScene(m_CameraController.GetCamera());

            //必须先缩放 再平移  cuz T*S != S*T.
            //Excepted behaviour: scale the object locally, then translate it to target world position.
            const auto scale = glm::scale(glm::mat4(1.0f), m_Scale);

            m_UniformBuffer->SetData(
                glm::value_ptr(m_SquareColor),
                sizeof(glm::vec3)
            );

            for (int y = 0; y < 20; y++)
            {
                for (int x = 0; x < 20; x++)
                {
                    m_Position = {0.11f * static_cast<float>(x), static_cast<float>(y) * 0.11f, 0.f};
                    const auto transform = glm::translate(glm::mat4(1.0f), m_Position);

                    Limen::Renderer::Submit(m_FlatColorShader, *m_SquareVAO, transform * scale);
                }
            }
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
            m_CameraController.OnEvent(e);
        }

        void OnImGuiRender() override
        {
            ImGui::Begin("Settings");
            ImGui::ColorEdit3("Square Color", glm::value_ptr(m_SquareColor));


            ImGui::End();
        }

    private:
        Limen::OrthoGraphicCameraController m_CameraController;

        // Ref默认是空指针；创建实际ShaderLibrary后才能调用Load()。
        Limen::Ref<Limen::ShaderLibrary> m_ShaderLib = Limen::CreateRef<Limen::ShaderLibrary>();

        Limen::Scope<Limen::VertexArray> m_SquareVAO;
        Limen::Scope<Limen::VertexArray> m_VertexArray;
        Limen::Ref<Limen::Shader> m_FlatColorShader, m_TextureShader;

        Limen::Ref<Limen::Texture2D> m_Texture, m_LogoTexture;

        Limen::Scope<Limen::UniformBuffer> m_UniformBuffer;

        Limen::Ref<Limen::VertexBuffer> m_VertexBuffer;
        Limen::Ref<Limen::VertexBuffer> m_SquareVBO;
        Limen::Ref<Limen::IndexBuffer> m_IndexBuffer;

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
            PushLayer(new SandBox::Example3DLayer());
        }

        ~SandBoxApp() override = default;
    };
}

//不实现是无法run的
Limen::Application *Limen::CreateApplication()
{
    return new SandBoxApp();
}
