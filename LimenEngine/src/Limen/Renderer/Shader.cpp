//
// Created by chenlong on 2026/8/11.
//
#include "Renderer/Shader.h"

#include "Log.h"
#include "Platform/Mac/OpenGL/OpenGLShader.h"
#include "Renderer/Renderer.h"

namespace Limen
{
    Ref<Shader> Shader::Create(
        const std::string &vertexSource,
        const std::string &fragmentSource)
    {
        switch (Renderer::GetRenderAPI())
        {
            case RendererAPI::API::OPENGL:
                return std::make_shared<OpenGLShader>(vertexSource, fragmentSource);

            case RendererAPI::API::DIRECT12:
                LM_CORE_ERROR("Cannot create a Shader when RenderAPI is NONE");
                return nullptr;

            case RendererAPI::API::NONE:
                LM_CORE_ERROR("Cannot create a Shader when RenderAPI is NONE");
                return nullptr;

            default:
                LM_CORE_ERROR("The selected RenderAPI does not implement Shader creation yet");
                return nullptr;
        }
    }
}
