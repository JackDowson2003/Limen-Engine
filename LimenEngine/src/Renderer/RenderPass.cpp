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
        :m_Specification(spec)
    {
        LM_CORE_ASSERT(m_Specification.TargetFramebuffer,"RenderPass '{}' requires a valid Framebuffer",
                    m_Specification.DebugName);
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

        /**
         * 选择该阶段的输出目标
         * Bind同时设置 Framebuffer 自己的 Viewport
         */
        m_Specification.TargetFramebuffer->Bind();

        /**
         *
         * 第一版 RenderPass固定清除颜色和深度
         * 后续会通过LoadOperation 决定 Clear、Load 或 DontCare
         *
         */
        RendererCommand::SetClearColor(m_Specification.ClearColor);

        RendererCommand::Clear();

        m_IsActive = true; // 完成 Bind 和 Clear 后，才把逻辑状态标记为 Active。

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

        /**
         * 第一版结束后回到默认的窗口渲染目标
         * 后续多Pass系统系统会修改为下一个渲染目标
         */
        m_Specification.TargetFramebuffer->UnBind();

        /*
         * Samples == 1 时 Resolve() 内部直接返回；
         * Samples > 1 时把 MSAA 结果解析到普通颜色纹理。
         */
        m_Specification.TargetFramebuffer->Resolve();

        m_IsActive = false;
    }
}
