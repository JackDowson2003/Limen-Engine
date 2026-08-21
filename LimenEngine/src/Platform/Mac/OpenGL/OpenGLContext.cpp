//
// Created by chenlong on 2026/8/10.
//

#include <glad/gl.h>
#include "OpenGLContext.h"
#include "Core.h"
#include "Log.h"
#include "Renderer/RendererCommand.h"

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
        glfwMakeContextCurrent(m_Window); //创建上下文

        //让 GLAD 找到并保存当前 OpenGL 驱动中各个 OpenGL 函数的地址
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

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(m_Window);
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
