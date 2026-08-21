#include "Limen/RHI/UniformBuffer.h"

#include "Limen/Core/Log.h"
#include "Limen/RHI/RendererAPI.h"
#include "RHI/macOS/OpenGL/OpenGLUniformBuffer.h"

namespace Limen
{
    Scope<UniformBuffer> UniformBuffer::Create(
        const uint32_t size,
        const uint32_t binding
    )
    {
        switch (RendererAPI::GetAPI())
        {
            case RendererAPI::API::OPENGL:
            {
#if defined(LIMEN_PLATFORM_MACOS)
                return CreateScope<OpenGLUniformBuffer>(
                    size,
                    binding
                );
#else
                LM_CORE_ERROR(
                    "Limen does not provide this OpenGL UniformBuffer backend"
                );
                return nullptr;
#endif
            }

            case RendererAPI::API::NONE:
                LM_CORE_ERROR(
                    "Cannot create UniformBuffer with RendererAPI::NONE"
                );
                return nullptr;

            default:
                LM_CORE_ERROR(
                    "UniformBuffer is not implemented for the selected RendererAPI"
                );
                return nullptr;
        }
    }
}