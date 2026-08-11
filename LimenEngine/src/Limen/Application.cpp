//
// Created by chenlong on 2026/8/5.
//
#include <glad/gl.h>

#include <memory>

#include "Application.h"

#include "Events/ApplicationEvent.h"
#include "ImGUI/ImGUILayer.h"
#include "Log.h"
#include "Platform/Mac/OpenGL/OpenGLBuffer.h"
#include "Renderer/Renderer.h"

namespace Limen
{
    Application *Application::s_Instance = nullptr;

    Application::Application()
    {
        LM_CORE_ASSERT(!s_Instance, "Application already initialized!");
        s_Instance = this;
        m_Window = std::unique_ptr<Window>(Window::Create());
        //发生Events的时候，就调用这个匿名函数,也就是OnEvent()
        m_Window->SetEventCallback([this](Event &e)
        {
            OnEvent(e);
        });

#if defined(LIMEN_PLATFORM_LINUX) || defined(LIMEN_PLATFORM_MACOS)
        // 编辑器 UI 由 Application 统一管理，客户端只需实现 OnImGuiRender()。
        m_ImGUILayer = new ImGUILayer();
        PushOverlay(m_ImGUILayer);
#endif

        constexpr float vertices[] = {
            -0.5f, -0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
            0.5f, -0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
            0.5f, 0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
        };

        //Vertex Array
        m_VertexArray.reset(VertexArray::Create());
        //Vertex Buffer
        m_VertexBuffer.reset(VertexBuffer::Create(vertices, sizeof(vertices)));
        const BufferLayout layout
        {
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float4, "a_Color"},
        };
        m_VertexBuffer->SetLayout(layout);
        m_VertexArray->AddVertexBuffer(m_VertexBuffer);

        //Index Buffer
        constexpr uint32_t indices[] = {0, 1, 2};
        m_IndexBuffer.reset(IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
        m_VertexArray->SetIndexBuffer(m_IndexBuffer);
        //====================
        m_SquareVAO.reset(VertexArray::Create());
        constexpr float SquareVertices[] = {
            0.5f, -0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
            0.7f, -0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
            0.7f, 0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
            0.5f, 0.5f, 0.0f, 1.f, 0.0f, 0.0f, 1.f,
        };

        m_VertexBuffer.reset(VertexBuffer::Create(SquareVertices, sizeof(SquareVertices)));
        m_VertexBuffer->SetLayout(layout);

        m_SquareVAO->AddVertexBuffer(m_VertexBuffer);

        constexpr uint32_t SquareIndices[] = {
            0, 1, 2,
            2, 3, 0
        };
        m_IndexBuffer.reset(IndexBuffer::Create(SquareIndices, sizeof(SquareIndices) / sizeof(uint32_t)));
        m_SquareVAO->SetIndexBuffer(m_IndexBuffer);


        //Shader
        const std::string vertexSource = R"(
            #version 410 core

            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec4 a_Color;

            out vec4 v_Color;
            out vec3 v_Position;

            void main()
            {
                // 裁剪空间坐标的 w 必须为 1，三角形才能正常进行透视除法。
                gl_Position = vec4(a_Position, 1.0);
                v_Color = a_Color;
                v_Position = a_Position;
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

        m_Shader = Shader::Create(vertexSource, fragmentSource);
        m_Shader->Bind();
        m_VertexArray->UnBind();
        m_Shader->UnBind();
        m_SquareVAO->UnBind();
    }


    Application::~Application()
    {
    }

    void Application::Run()
    {
        while (m_Running)
        {
            // 深色背景能清楚显示亮蓝色三角形。
            RendererCommand::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});
            RendererCommand::Clear();

            Renderer::BeginScene();
            m_Shader->Bind();

            Renderer::Submit(m_VertexArray); //draw

            Renderer::Submit(m_SquareVAO); //draw
            Renderer::EndScene();

            // Application.cpp
            for (Layer *layer: m_LayerStack)
                layer->OnUpdate();

#if defined(LIMEN_PLATFORM_LINUX) || defined(LIMEN_PLATFORM_MACOS)
            LM_CORE_ASSERT(m_ImGUILayer, "ImGui layer was not initialized");
            ImGUILayer::Begin();

            for (Layer *&layer: m_LayerStack)
                layer->OnImGuiRender();

            ImGUILayer::End();
#endif

            m_Window->OnUpdate();
        }
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent &e)
        {
            return OnWindowClose(e);
        });
        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            if (e.IsHandled())
                break;

            (*it)->OnEvent(e);
        }
        // LM_CORE_TRACE("{0}",e.ToString());
    }

    bool Application::OnWindowClose(WindowCloseEvent &e)
    {
        m_Running = false;
        return true;
    }

    void Application::PushLayer(Layer *layer)
    {
        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer *layer)
    {
        m_LayerStack.PushOverlay(layer);
        layer->OnAttach();
    }
}
