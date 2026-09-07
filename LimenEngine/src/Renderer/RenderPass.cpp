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

        // 记录本次 Begin() 真正需要清理的附件。
        ClearFlags clearFlags = ClearFlags::None;

        if (m_Specification.ColorLoadOperation == AttachmentLoadOperation::Clear)
        {
            RendererCommand::SetClearColor(m_Specification.ClearColor);
            clearFlags |= ClearFlags::Color;
        }

        // 当前FBO使用的是打包的Depth24Stencil8附件
        // 因此要求清理深度模版时，同时清理Depth和Stencil
        if (m_Specification.DepthStencilLoadOperation == AttachmentLoadOperation::Clear)
        {
            clearFlags |= ClearFlags::Depth;
            clearFlags |= ClearFlags::Stencil;
        }

        // Load指令和 DontCare都不会产生清理指令
        // ClearFlags::None 时也没必要调用底层的清理指令
        if (clearFlags != ClearFlags::None)
            RendererCommand::Clear(clearFlags);

        // 只有 Bind 与 Clear 都完成后，该 Pass 才处于活动状态。
        m_IsActive = true;
    }

    void RenderPass::End()
    {
        if (!m_IsActive)
        {
            LM_ERROR("RenderPass '{}' ended without Begin", m_Specification.DebugName);
            return;
        }

        /**
         * Store 表示后续还需要使用本次Pass产生的颜色
         *
         * MSAA 下需要把多采样 Resolve 到普通颜色纹理
         * 单采样下 Resolve() 不执行任何操作
         */
        if (m_Specification.ColorStoreOperation == AttachmentStoreOperation::Store)
            m_Specification.TargetFramebuffer->Resolve();

        // 当前版本回到窗口默认 Framebuffer；多 Pass 系统将改为绑定下一目标。
        m_Specification.TargetFramebuffer->UnBind();

        m_IsActive = false;
    }
}
