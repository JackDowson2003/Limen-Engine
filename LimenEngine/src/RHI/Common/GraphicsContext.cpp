#include "RHI/Common/GraphicsContext.h"

#include "Limen/Core/Log.h"
#include "Limen/RHI/RendererAPI.h"

#if defined(LIMEN_PLATFORM_MACOS)
#include "RHI/macOS/OpenGL/OpenGLContext.h"
#include <GLFW/glfw3.h>
#endif

namespace Limen
{
    Scope<GraphicsContext> GraphicsContext::Create(void* nativeWindow)
    {
        LM_CORE_ASSERT(nativeWindow, "Cannot create GraphicsContext for a null window");
        if (!nativeWindow)
            return nullptr;

        switch (RendererAPI::GetAPI())
        {
            case RendererAPI::API::OPENGL:
            {
#if defined(LIMEN_PLATFORM_MACOS)
                return CreateScope<OpenGLContext>(
                    static_cast<GLFWwindow*>(nativeWindow)
                );
#else
                LM_CORE_ERROR("OpenGLContext is not implemented on this platform yet");
                return nullptr;
#endif
            }

            case RendererAPI::API::METAL:
            {
#if defined(LIMEN_PLATFORM_MACOS)
                LM_CORE_ERROR(
                    "Metal was selected, but MetalContext is not implemented yet"
                );
#else
                LM_CORE_ERROR("Metal is only available on Apple platforms");
#endif
                return nullptr;
            }

            case RendererAPI::API::DIRECT11:
            case RendererAPI::API::DIRECT12:
                LM_CORE_ERROR("Direct3D GraphicsContext is only supported on Windows");
                return nullptr;

            case RendererAPI::API::VULKAN:
                LM_CORE_ERROR("VulkanContext is not implemented yet");
                return nullptr;

            case RendererAPI::API::NONE:
                LM_CORE_ERROR("Cannot create GraphicsContext with RendererAPI::NONE");
                return nullptr;
        }

        LM_CORE_ERROR("Unknown RendererAPI value");
        return nullptr;
    }
}
