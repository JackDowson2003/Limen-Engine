//
// Created by chenlong on 2026/8/10.
//

#include <glad/gl.h>
#include "OpenGLContext.h"
#include "Limen/Core/Core.h"
#include "Limen/Core/Log.h"

namespace Limen
{
    OpenGLContext::OpenGLContext(GLFWwindow *windowHandle)
    {
        LM_CORE_ASSERT(windowHandle, "Window handle is null, cannot create context");
        m_Window = windowHandle;

    }

    OpenGLContext::~OpenGLContext()
    {
        OpenGLContext::Shutdown();
    }

    void OpenGLContext::Init()
    {
        // 后续所有 OpenGL 调用都作用于这个线程的当前上下文。
        glfwMakeContextCurrent(m_Window);

        // 通过 GLFW 查询驱动函数地址，并填充 GLAD 的 OpenGL 函数指针。
        const int status = gladLoadGL(glfwGetProcAddress);
        LM_CORE_ASSERT(status, "Failed to initialize OpenGL context");
        if (!status)
            return;

        const auto *vendor =reinterpret_cast<const char *>(glGetString(GL_VENDOR));
        const auto *version =reinterpret_cast<const char *>(glGetString(GL_VERSION));
        const auto *renderer =reinterpret_cast<const char *>(glGetString(GL_RENDERER));
        LM_CORE_INFO("OpenGL Info :");
        LM_CORE_INFO("Vendor: {}", vendor ? vendor : "Unknown");
        LM_CORE_INFO("Version: {}", version );
        LM_CORE_INFO("Renderer: {}", renderer ? renderer : "Unknown");
    }

    void OpenGLContext::Present()
    {
        glfwSwapBuffers(m_Window);
    }

    void OpenGLContext::SetVSync(const bool enabled)
    {
        glfwSwapInterval(enabled ? 1 : 0);
    }

    void OpenGLContext::Shutdown()
    {
        if (!m_Window)
            return;

        // OpenGLContext只借用窗口句柄。这里解除当前上下文，
        // GLFWwindow的实际销毁由MacWindow负责。
        if (glfwGetCurrentContext() == m_Window)
            glfwMakeContextCurrent(nullptr);

        m_Window = nullptr;
    }
}
