//
// Created by chenlong on 2026/8/7.
//

#include "Paltform/Mac/MacWindow.h"

#include "Log.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

namespace Limen
{
    static bool s_GLFWInitialized = false;

    static void GLFWErrorCallback(int error_code, const char* description)
    {
        LM_CORE_ERROR("GLFW Error: {0} {1}",error_code, description);
    }


    MacWindow::MacWindow(const WindowProps &props)
    {
        MacWindow::Init(props);
    }

    MacWindow::~MacWindow()
    {
        MacWindow::Shutdown();
    }

    bool MacWindow::IsVSync() const
    {
        return m_Data.VSync;
    }

    void MacWindow::OnUpdate()
    {
        glfwPollEvents(); //处理事件
        //交换前后缓冲区，开启Vsync。调用glfwSwapBuffers的时候，会阻塞等待显示器垂直刷新信号，才交换缓冲，消除画面撕裂，锁帧率。
        glfwSwapBuffers(m_Window);
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

        LM_CORE_INFO("Creating window {0} {1} {2}", props.Title, props.Width, props.Height);
        if (!s_GLFWInitialized)
        {
            //TODO: glfwTerminate on system shutdown
            const int success = glfwInit();
            LM_CORE_ASSERT(success, "Could not initialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);
            s_GLFWInitialized = true;
        }

        m_Window = glfwCreateWindow(static_cast<int>(props.Width), static_cast<int>(props.Height),
                                    props.Title.c_str(),
                                    nullptr, nullptr);

        glfwMakeContextCurrent(m_Window); //创建上下文

        glfwSetWindowUserPointer(m_Window, &m_Data);
        SetVSync(true); //垂直同步

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
            WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
            WindowCloseEvent event;
            data.EventCallBack(event);
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
        {
            WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
            switch (action)
            {
                case GLFW_PRESS:
                {
                    KeyPressedEvent event(key, 0);
                    data.EventCallBack(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent event(key);
                    data.EventCallBack(event);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent event(key, 1);
                    data.EventCallBack(event);
                    break;
                }
                default: break;
            }
        });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow *window, int button, int action, int mods)
        {
            WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
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
        glfwDestroyWindow(m_Window);
    }
}
