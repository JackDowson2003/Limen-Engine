//
// Created by chenlong on 2026/8/12.
//
#include "Limen/Renderer/RendererCommand.h"

#include "Limen/Core/Log.h"

#if defined(LIMEN_PLATFORM_MACOS) || defined(LIMEN_PLATFORM_LINUX)
#include "RHI/macOS/OpenGL/OpenGLRendererAPI.h"
#endif

namespace Limen
{
    namespace
    {
        /**
         * @brief 根据运行时选择的API和编译目标平台创建后端实现。
         *
         * 当前只有macOS/Linux OpenGL分支拥有真实实现；Windows的
         * Direct11/Direct12分支会在对应后端加入后接入这里。
         */
        [[nodiscard]]
        Scope<RendererAPI> CreateRendererAPI()
        {
            switch (RendererAPI::GetAPI())
            {
                case RendererAPI::API::OPENGL:
                {
#if defined(LIMEN_PLATFORM_MACOS) || defined(LIMEN_PLATFORM_LINUX)
                    return CreateScope<OpenGLRendererAPI>();
#elif defined(LIMEN_PLATFORM_WINDOWS)
                    LM_CORE_ERROR("The Windows OpenGL backend is not implemented yet");
                    return nullptr;
#else
                    LM_CORE_ERROR("OpenGL is not supported on this platform");
                    return nullptr;
#endif
                }

                case RendererAPI::API::DIRECT11:
                {
#if defined(LIMEN_PLATFORM_WINDOWS)
                    LM_CORE_ERROR("The Direct3D 11 backend is not implemented yet");
#else
                    LM_CORE_ERROR("Direct3D 11 is only available on Windows");
#endif
                    return nullptr;
                }

                case RendererAPI::API::DIRECT12:
                {
#if defined(LIMEN_PLATFORM_WINDOWS)
                    LM_CORE_ERROR("The Direct3D 12 backend is not implemented yet");
#else
                    LM_CORE_ERROR("Direct3D 12 is only available on Windows");
#endif
                    return nullptr;
                }

                case RendererAPI::API::VULKAN:
                    LM_CORE_ERROR("The Vulkan backend is not implemented yet");
                    return nullptr;

                case RendererAPI::API::METAL:
                    LM_CORE_ERROR("The Metal backend is not implemented yet");
                    return nullptr;

                case RendererAPI::API::NONE:
                    LM_CORE_ERROR("Cannot initialize RendererCommand with RendererAPI::NONE");
                    return nullptr;
            }

            LM_CORE_ERROR("Unknown RendererAPI value");
            return nullptr;
        }
    }

    Scope<RendererAPI> RendererCommand::s_RendererAPI;

    void RendererCommand::Init()
    {
        if (s_RendererAPI)
        {
            LM_CORE_WARN("RendererCommand is already initialized");
            return;
        }

        s_RendererAPI = CreateRendererAPI();

        LM_CORE_ASSERT(
            s_RendererAPI,
            "Failed to create the selected RendererAPI backend"
        );

        if (s_RendererAPI)
            s_RendererAPI->Init();
    }

    void RendererCommand::Shutdown()
    {
        s_RendererAPI.reset();
    }

    RendererAPI* RendererCommand::GetRendererAPI() noexcept
    {
        if (!s_RendererAPI)
            LM_CORE_ERROR("RendererCommand was used before Init() or after Shutdown()");

        return s_RendererAPI.get();
    }
}
