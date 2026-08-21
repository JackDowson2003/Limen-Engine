//
// Created by chenlong on 2026/8/19.
//
#include "Limen/RHI/Texture.h"

#include "Limen/Core/Log.h"
#include "RHI/macOS/OpenGL/OpenGLTexture2D.h"
#include "Limen/Renderer/Renderer.h"

Limen::Ref<Limen::Texture2D> Limen::Texture2D::Create(const char *path)
{
    switch (Renderer::GetRenderAPI())
    {
        case RendererAPI::API::NONE:
        {
            LM_ERROR("Cannot create texture, the API is NONE type !!!");
            return nullptr;
        }
        case RendererAPI::API::OPENGL:
            return OpenGLTexture2D::LoadFromFile(path);
        case RendererAPI::API::DIRECT12:
            return nullptr;
        default:
            return nullptr;
    }
}
