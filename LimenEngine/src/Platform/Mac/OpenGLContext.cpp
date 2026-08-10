//
// Created by chenlong on 2026/8/10.
//

#include <glad/gl.h>
#include "OpenGLContext.h"
#include "Core.h"
#include "Log.h"

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

        const int status = gladLoadGL(glfwGetProcAddress);
        LM_CORE_ASSERT(status, "Failed to initialize OpenGL context");
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(m_Window);
    }

    void OpenGLContext::Shutdown()
    {
        glfwDestroyWindow(m_Window);
    }
}
