//
// Created by chenlong on 2026/8/7.
//
#include "Platform/macOS/MacWindow.h"

#include "Limen/Core/Log.h"
#include "Limen/RHI/RendererAPI.h"
#include "Limen/Events/ApplicationEvent.h"
#include "Limen/Events/KeyEvent.h"
#include "Limen/Events/MouseEvent.h"
#include "Platform/GLFW/GLFWKeyCodes.h"
namespace Limen
{
    static bool s_GLFWInitialized = false;

    static void GLFWErrorCallback(int error_code, const char* description)
    {
        LM_CORE_ERROR("GLFW Error: {0} {1}",error_code, description);
    }



    MacWindow::MacWindow(const WindowProps &props)
    {
        Init(props);
    }

    MacWindow::~MacWindow()
    {
        Shutdown();
    }

    bool MacWindow::IsVSync() const
    {
        return m_Data.VSync;
    }

    void MacWindow::PollEvents()
    {
        glfwPollEvents();
    }

    void MacWindow::Present()
    {
        LM_CORE_ASSERT(m_Context, "Cannot present without a GraphicsContext");

        if (m_Context)
            m_Context->Present();
    }

    void MacWindow::SetVSync(const bool enabled)
    {
        LM_CORE_ASSERT(m_Context, "Cannot configure VSync without a GraphicsContext");
        if (m_Context)
            m_Context->SetVSync(enabled);

        m_Data.VSync = enabled;
    }

    void MacWindow::Init(const WindowProps &props)
    {
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        LM_CORE_INFO("Create Window Title: {0}, Width: {1}, Height :{2}", props.Title, props.Width, props.Height);
        LM_CORE_TRACE("Creating Window Now");

        LM_CORE_INFO("Creating window {0} {1} {2}", props.Title, props.Width, props.Height);
        if (!s_GLFWInitialized)
        {
            //TODO: glfwTerminate on system shutdown
            const int success = glfwInit();
            LM_CORE_ASSERT(success, "Could not initialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);
            s_GLFWInitialized = true;
        }

        // GLFW的窗口提示具有全局状态，先恢复默认值，再按本次API配置。
        glfwDefaultWindowHints();

        switch (RendererAPI::GetAPI())
        {
            case RendererAPI::API::OPENGL:
                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
                glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
                break;

            case RendererAPI::API::METAL:
                // Metal不使用GLFW创建的OpenGL Context，只需要原生窗口。
                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                break;

            default:
                LM_CORE_ERROR("The selected RendererAPI cannot create a macOS window surface");
                LM_CORE_ASSERT(false, "Unsupported RendererAPI on macOS");
                return;
        }

        m_Window = glfwCreateWindow(static_cast<int>(props.Width), static_cast<int>(props.Height),
                                    props.Title.c_str(),
                                    nullptr, nullptr);

        LM_CORE_ASSERT(m_Window, "Failed to create GLFW window");
        if (!m_Window)
            return;

        // Context的具体类型由RendererAPI决定，而不是由MacWindow写死。
        m_Context = GraphicsContext::Create(m_Window);
        LM_CORE_ASSERT(m_Context, "Failed to create the selected GraphicsContext");
        if (!m_Context)
            return;

        m_Context->Init();

        glfwSetWindowUserPointer(m_Window, &m_Data);
        SetVSync(true); //垂直同步


        glfwSetCharCallback(m_Window, [](GLFWwindow *window, unsigned int codepoint)
        {
            const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
            KeyTypedEvent event(static_cast<char32_t>(codepoint));
            data.EventCallBack(event);
        });

        //Set GLFW Callbacks
        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow *window, int width, int height)
        {
            const auto cWidth = static_cast<unsigned int>(width);
            const auto cHeight = static_cast<unsigned int>(height);
            WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
            WindowResizeEvent event(cWidth, cHeight);
            data.EventCallBack(event);
            data.Width = cWidth;
            data.Height = cHeight;
        });

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow *window)
        {
            const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
            WindowCloseEvent event;
            data.EventCallBack(event);
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
        {
            const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
            const KeyCode keyCode = KeyCodeFromGLFW(key);
            if (keyCode == KeyCode::Unknown)
                return;

            switch (action)
            {
                case GLFW_PRESS:
                {
                    KeyPressedEvent event(keyCode, 0);
                    data.EventCallBack(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent event(keyCode);
                    data.EventCallBack(event);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent event(keyCode, 1);
                    data.EventCallBack(event);
                    break;
                }
                default: break;
            }
        });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow *window, int button, int action, int mods)
        {
            const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
            switch (action)
            {
                case GLFW_PRESS:
                {
                    MouseButtonPressedEvent event(button);
                    data.EventCallBack(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    MouseButtonReleasedEvent event(button);
                    data.EventCallBack(event);
                    break;
                }
                default: ;
            }
        });

        glfwSetScrollCallback(m_Window, [](GLFWwindow *window, double xoffset, double yoffset)
        {
            const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
            const auto uXOffset = static_cast<float>(xoffset);
            const auto uYOffset = static_cast<float>(yoffset);

            MouseScrolledEvent event(uXOffset, uYOffset);

            /*
             *在我们的Application中设置了怎么用的 也就是
             *m_Window->SetEventCallback([this](Event& e)
              {
                OnEvent(e);
              });
            */
            data.EventCallBack(event);
        });

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow *window, double xpos, double ypos)
        {
            const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
            const auto x = static_cast<float>(xpos);
            const auto y = static_cast<float>(ypos);

            MouseMovedEvent event(x, y);

            data.EventCallBack(event);
        });
    }

    void MacWindow::Shutdown()
    {
        // 先销毁Context包装对象，使它不再引用即将销毁的GLFWwindow。
        m_Context.reset();

        // Native Window只由MacWindow销毁。
        if (m_Window)
        {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }

        // 当前引擎只支持一个主窗口；多窗口阶段再改为引用计数式WindowSystem。
        if (s_GLFWInitialized)
        {
            glfwTerminate();
            s_GLFWInitialized = false;
        }
    }
}
