//
// Created by chenlong on 2026/8/5.
//

#include <chrono>

#include "Limen/Application/Application.h"
#include "Limen/Events/ApplicationEvent.h"
#include "Editor/ImGui/ImGUILayer.h"
#include "Limen/Core/Log.h"
#include "GLFW/glfw3.h"
#include "Limen/Asset/AssetManager.h"
#include "Limen/Renderer/Renderer.h"

namespace Limen
{
    Application *Application::s_Instance = nullptr;

    /**
     * @brief 创建应用，并在窗口创建前选择图形 API。
     *
     * @param isVSYNC 是否启用垂直同步。
     * @param rendererAPI 本次运行使用的图形 API。
     * @param enableImGuiDebugLayer 是否启用 Sandbox 使用的 ImGui 调试层；
     *                              正式 Editor 默认不启用。
     * @param assetRoot 资源根
     */
    Application::Application(
        const bool isVSYNC,
        const RendererAPI::API rendererAPI,
        const bool enableImGuiDebugLayer,
        const std::filesystem::path& assetRoot
    )
    {
        LM_CORE_ASSERT(!s_Instance, "Application already initialized!");
        s_Instance = this;

        // 图形 API 必须先于 Window/GraphicsContext 确定；同一个 MacWindow
        // 以后可以根据该选择承载 OpenGLContext 或 MetalContext。
        RendererAPI::SetAPI(rendererAPI);

        m_Window = Scope<Window>(Window::Create());
        m_Window->SetVSync(isVSYNC);
        // Window 将原生事件转换为引擎 Event 后，通过该回调交给 Application。
        m_Window->SetEventCallback([this](Event &e)
        {
            OnEvent(e);
        });

        Renderer::Init();

        AssetManager::Init(assetRoot);

#if defined(LIMEN_PLATFORM_LINUX) || defined(LIMEN_PLATFORM_MACOS)
        if (enableImGuiDebugLayer)
        {
            // 调试 UI 由客户端显式选择；LayerStack 拥有该 Layer。
            m_ImGUILayer = new ImGUILayer();
            PushOverlay(m_ImGUILayer);
        }
#else
        // 这些平台当前没有启用该调试层，避免未使用参数警告。
        (void) enableImGuiDebugLayer;
#endif
    }


    Application::~Application()
    {
        // Layer 中的 GPU 资源必须先于 Renderer 后端与 Window/Context 释放。
        m_LayerStack.Clear();
        m_ImGUILayer = nullptr;

        // 再释放资源管理器缓存中剩余的GPU资源。
        AssetManager::Shutdown();

        Renderer::Shutdown();

        // 最后销毁图形 Context 与原生窗口。
        m_Window.reset();
        s_Instance = nullptr;
    }

    void Application::Run()
    {
        m_LastFrameTime = glfwGetTime();
        while (m_Running)
        {
            // 帧首获取最新的窗口、键盘和鼠标事件。
            m_Window->PollEvents();

            if (!m_Running)
                break;


            const auto cpuFrameWorkStart = std::chrono::steady_clock::now();

            Renderer::BeginFrame();
            const double time = glfwGetTime();
            DeltaTime deltaTime = time - m_LastFrameTime;
            m_LastFrameTime = time;
            if (!m_Minimized)
            {
                for (Layer *layer: m_LayerStack)
                    layer->OnUpdate(deltaTime); //更新数据

                /*
                 * 各 Layer 完成当前的离屏场景绘制后，清理窗口默认 Framebuffer。
                 * Sandbox 启用调试层时，它会在清理后绘制；
                 * 正式 Editor 的 C++ UI 绘制阶段将在后续单独接入。
                 */
                RendererCommand::SetClearColor(
                    {0.04f, 0.04f, 0.04f, 1.0f}
                );
                RendererCommand::Clear();
                // 所有场景更新结束、窗口清屏之后，才允许提交正式 UI。
                for (const auto &layer: m_LayerStack)
                    layer->OnUIRender(*m_Window);
            }

#if defined(LIMEN_PLATFORM_LINUX) || defined(LIMEN_PLATFORM_MACOS)
            if (m_ImGUILayer)
            {
                m_ImGUILayer->Begin();
                for (Layer *layer: m_LayerStack)
                    layer->OnImGuiRender();
                m_ImGUILayer->End();
            }
#endif

            Renderer::EndFrame();
            const auto cpuFrameWorkEnd = std::chrono::steady_clock::now();
            m_LastCPUFrameWorkMilliseconds =
                    std::chrono::duration<double, std::milli>(
                        cpuFrameWorkEnd - cpuFrameWorkStart
                    ).count();
            // 所有场景与 ImGui 命令完成后再交换窗口缓冲。
            m_Window->Present(); //render
        }
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent &e)
        {
            return OnWindowClose(e);
        });
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent &e)
        {
            return OnWindowResize(e);
        });
        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            if (e.IsHandled())
                break;

            (*it)->OnEvent(e);
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent &e)
    {
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent &e)
    {
        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            m_Minimized = true;
            return false;
        }
        m_Minimized = false;
        Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
        return false;
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
