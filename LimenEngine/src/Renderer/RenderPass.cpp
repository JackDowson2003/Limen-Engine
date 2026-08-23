//
// Created by chenlong on 2026/8/23.
//
#include "Limen/Renderer/RenderPass.h"

#include "Limen/Core/Log.h"
#include "Limen/Renderer/RendererCommand.h"
#include "Limen/RHI/Framebuffer.h"

namespace Limen
{
    RenderPass::RenderPass(const RenderPassSpecification &spec)
        : m_Specification(spec)
    {
        LM_CORE_ASSERT(
            m_Specification.TargetFramebuffer,
            "RenderPass '{}' requires a valid Framebuffer",
            m_Specification.DebugName
        );
    }

    void RenderPass::Begin()
    {
        if (m_IsActive)
        {
            LM_CORE_ASSERT(
                false,
                "RenderPass '{}' is already active",
                m_Specification.DebugName
            );
            return;
        }
        if (!m_Specification.TargetFramebuffer)
        {
            LM_CORE_ASSERT(
                false,
                "RenderPass '{}' has no target Framebuffer",
                m_Specification.DebugName
            );
            return;
        }

        // Bind() 同时把 Viewport 更新为目标 Framebuffer 的尺寸。
        m_Specification.TargetFramebuffer->Bind();

        // 当前版本固定清除颜色和深度；未来由 LoadOperation 决定行为。
        RendererCommand::SetClearColor(m_Specification.ClearColor);
        RendererCommand::Clear();

        // 只有 Bind 与 Clear 都完成后，该 Pass 才处于活动状态。
        m_IsActive = true;
    }

    void RenderPass::End()
    {
        if (!m_IsActive)
        {
            LM_CORE_ASSERT(
                false,
                "RenderPass '{}' ended without Begin",
                m_Specification.DebugName
            );
            return;
        }

        // 当前版本回到窗口默认 Framebuffer；多 Pass 系统将改为绑定下一目标。
        m_Specification.TargetFramebuffer->UnBind();

        // 单采样时 Resolve() 直接返回；多采样时解析到普通颜色纹理。
        m_Specification.TargetFramebuffer->Resolve();

        m_IsActive = false;
    }
}
