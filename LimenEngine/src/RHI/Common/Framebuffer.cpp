#include "Limen/RHI/Framebuffer.h"

#include "Limen/Core/Log.h"
#include "Limen/RHI/RendererAPI.h"

#if defined(LIMEN_PLATFORM_MACOS)
    #include "RHI/macOS/OpenGL/OpenGLFramebuffer.h"
#endif

namespace Limen
{
    Scope<Framebuffer> Framebuffer::Create(
        const FramebufferSpecification& specification
    )
    {
        if (
            specification.Width == 0 ||
            specification.Height == 0
        )
        {
            LM_CORE_ERROR(
                "Framebuffer size must be greater than zero. Width={}, Height={}",
                specification.Width,
                specification.Height
            );
            return nullptr;
        }

        if (specification.Samples == 0)
        {
            LM_CORE_ERROR(
                "Framebuffer sample count must be at least one"
            );
            return nullptr;
        }

        switch (RendererAPI::GetAPI())
        {
            case RendererAPI::API::OPENGL:
            {
#if defined(LIMEN_PLATFORM_MACOS)
                return CreateScope<OpenGLFramebuffer>(
                    specification
                );
#else
                LM_CORE_ERROR(
                    "OpenGL Framebuffer is unavailable on this platform"
                );
                return nullptr;
#endif
            }

            case RendererAPI::API::NONE:
            {
                LM_CORE_ERROR(
                    "Cannot create Framebuffer when RendererAPI is NONE"
                );
                return nullptr;
            }

            default:
            {
                LM_CORE_ERROR(
                    "Framebuffer is not implemented for the selected RendererAPI"
                );
                return nullptr;
            }
        }
    }
}