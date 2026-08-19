//
// Created by chenlong on 2026/8/7.
//
#include <glad/gl.h> //glad要在glfw钱
#include "Platform/Mac/MacWindow.h"

#include "Log.h"
#include "OpenGL/OpenGLContext.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "GLFW/GLFWKeyCodes.h"
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

    void MacWindow::OnUpdate()
    {
        glfwPollEvents(); //处理事件
        m_Context->SwapBuffers(); ////交换前后缓冲区
        //交换前后缓冲区，开启Vsync。调用glfwSwapBuffers的时候，会阻塞等待显示器垂直刷新信号，才交换缓冲，消除画面撕裂，锁帧率。
        // glfwSwapBuffers(m_Window);
    }

    void MacWindow::SetVSync(const bool enabled)
    {
        if (enabled)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
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

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); //opengl 4
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); //opengl 3
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //core mode只用核心功能，不使用兼容的

#if defined(LIMEN_PLATFORM_MACOS)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); //我连核心模式里暂时残留的、未来会删除的东西也不用，直接当它们不存在
#endif

        m_Window = glfwCreateWindow(static_cast<int>(props.Width), static_cast<int>(props.Height),
                                    props.Title.c_str(),
                                    nullptr, nullptr);
        m_Context = new OpenGLContext(m_Window);

        // glfwMakeContextCurrent(m_Window); //创建上下文
        //获取opengl的函数

        // const int status = gladLoadGL(glfwGetProcAddress);
        // LM_CORE_ASSERT(status, "Failed to initialize GLAD!");
        // if (!status)
        // {
        //     Shutdown();
        //     return;
        // }
        m_Context->Init(); //初始化上下文

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

    void MacWindow::Shutdown() const
    {
        m_Context->Shutdown();
        // glfwDestroyWindow(m_Window);
        glfwTerminate();
    }
}
