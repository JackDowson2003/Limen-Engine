//
// Created by chenlong on 2026/8/10.
//
#pragma once
#include "GLFW/glfw3.h"
#include "RHI/Common/GraphicsContext.h"

namespace Limen
{
    class OpenGLContext : public GraphicsContext
    {
    public:
        explicit OpenGLContext(GLFWwindow* windowHandle);

        ~OpenGLContext() override;

        void Init() override;

        void Present() override;

        void SetVSync(bool enabled) override;

        void Shutdown() override;

    private:
        GLFWwindow* m_Window{};
    };
}
