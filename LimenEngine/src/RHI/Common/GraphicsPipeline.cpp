//
// Created by chenlong on 2026/8/23.
//

#include "Limen/RHI/GraphicsPipeline.h"

#include "Limen/Core/Log.h"
#include "Limen/RHI/RendererAPI.h"

#if defined(LIMEN_PLATFORM_MACOS)
    #include "RHI/macOS/OpenGL/OpenGLGraphicsPipeline.h"
#endif

namespace Limen
{
    Ref<GraphicsPipeline> GraphicsPipeline::Create(const GraphicsPipelineSpecification &specification)
    {
        // 在公共工厂处拒绝无效规格，避免创建一个永远无法绑定的后端对象。
        if (!specification.ShaderProgram)
        {
            LM_CORE_ERROR("Cannot create graphics pipeline '{}': shader is null", specification.DebugName);

            return nullptr;
        }

        switch (RendererAPI::GetAPI())
        {
            case RendererAPI::API::OPENGL:
            {
#if defined(LIMEN_PLATFORM_MACOS)
                return CreateRef<OpenGLGraphicsPipeline>(
                    specification
                );
#else
                LM_CORE_ERROR(
                    "OpenGL graphics pipeline is unavailable on this platform"
                );

                return nullptr;
#endif
            }

            case RendererAPI::API::DIRECT11:
            {
                LM_CORE_ERROR(
                    "Direct3D 11 graphics pipeline is not implemented"
                );

                return nullptr;
            }

            case RendererAPI::API::DIRECT12:
            {
                LM_CORE_ERROR(
                    "Direct3D 12 graphics pipeline is not implemented"
                );

                return nullptr;
            }

            case RendererAPI::API::METAL:
            {
                LM_CORE_ERROR(
                    "Metal graphics pipeline is not implemented"
                );

                return nullptr;
            }

            case RendererAPI::API::VULKAN:
            {
                LM_CORE_ERROR(
                    "Vulkan graphics pipeline is not implemented"
                );

                return nullptr;
            }

            case RendererAPI::API::NONE:
            {
                LM_CORE_ERROR(
                    "Cannot create graphics pipeline when RendererAPI is NONE"
                );

                return nullptr;
            }
        }
        LM_CORE_ERROR(
            "Unknown RendererAPI while creating graphics pipeline"
        );

        return nullptr;
    }
}
