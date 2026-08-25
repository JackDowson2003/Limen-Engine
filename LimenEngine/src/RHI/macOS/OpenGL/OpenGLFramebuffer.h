#pragma once

#include <cstdint>

#include "Limen/RHI/Framebuffer.h"

namespace Limen
{
    class OpenGLFramebuffer final : public Framebuffer
    {
    public:
        /**
         * @param specification Framebuffer 的创建参数。
         */
        explicit OpenGLFramebuffer(
            const FramebufferSpecification& specification
        );

        ~OpenGLFramebuffer() override;

        void Bind() const override;

        void UnBind() const override;

        void Resolve() const override;

        void Resize(
            uint32_t width,
            uint32_t height
        ) override;

        [[nodiscard]]
        const FramebufferSpecification&
        GetSpecification() const noexcept override
        {
            return m_Specification;
        }

        [[nodiscard]]
        std::uintptr_t
        GetColorAttachmentHandle() const noexcept override
        {
            return m_ColorAttachment;
        }

    private:
        /**
         * 删除旧附件，再按当前 Specification 重建
         * 只负责创建和连接资源，不负责绘制场景
         */
        void Invalidate();

        /**
         * 删除当前 OpenGL FBO、颜色纹理和 Renderbuffer。
         */
        void Release();

    private:
        /**
         * 当前 Framebuffer 的尺寸和 MSAA 采样数。
         *
         * Resize() 会修改 Width、Height；如果用户请求的 Samples
         * 超过显卡上限，Invalidate() 会把它限制到硬件支持的最大值。
         */
        FramebufferSpecification m_Specification;

        /**
         * 场景渲染 FBO 的 OpenGL 对象 ID。
         *
         * FBO 全称是 Framebuffer Object（帧缓冲对象）。
         * 它决定了 GPU 绘制出来的颜色、深度和模板结果要写到哪里
         *
         * FBO 自己不保存像素，它只负责把颜色、深度等附件组织成
         * 一个渲染目标。Bind() 绑定的就是这个 ID，之后的 Clear、
         * DrawIndexed 都会写入挂在它上面的附件。
         *
         * Samples == 1：
         *   颜色附件是 m_ColorAttachment；
         *   深度附件是 m_DepthStencilRenderbuffer。
         *
         * Samples > 1：
         *   颜色附件是 m_MultisampleColorRenderbuffer；
         *   深度附件是 m_DepthStencilRenderbuffer。
         */
        uint32_t m_RendererID = 0;

        /**
         * MSAA Resolve 目标 FBO 的 OpenGL 对象 ID。
         *
         * 仅 Samples > 1 时创建。它挂载普通的 m_ColorAttachment，
         * Resolve() 使用 glBlitFramebuffer() 把多采样颜色结果从
         * m_RendererID 解析到这里。
         *
         * Samples == 1 时不需要 Resolve，因此该字段保持为 0。
         */
        uint32_t m_ResolveFramebufferID = 0;

        /**
         * 最终场景颜色纹理的 OpenGL Texture ID。
         *
         * 它始终是普通的单采样 GL_TEXTURE_2D，因此可以被 Shader
         * 采样，也可以通过 GetColorAttachmentHandle() 交给
         * ImGui::Image() 显示。
         *
         * Samples == 1：场景直接渲染到这张纹理。
         * Samples > 1：它接收 Resolve 后的最终颜色。
         */
        uint32_t m_ColorAttachment = 0;

        /**
         * 多采样颜色 Renderbuffer 的 OpenGL 对象 ID。
         *
         * 仅 Samples > 1 时创建，并作为 m_RendererID 的颜色附件。
         * 它为每个像素保存多个颜色样本，用来减少几何边缘锯齿。
         *
         * 多采样 Renderbuffer 不能直接交给 ImGui 或普通 sampler2D，
         * 所以绘制结束后必须 Resolve 到 m_ColorAttachment。
         */
        uint32_t m_MultisampleColorRenderbuffer = 0;

        /**
         * 深度与模板 Renderbuffer 的 OpenGL 对象 ID。
         *
         * 格式为 GL_DEPTH24_STENCIL8：每个像素包含 24 位深度值和
         * 8 位模板值。3D 深度测试依赖它来判断前后遮挡。
         *
         * Samples == 1 时创建普通 Renderbuffer；Samples > 1 时
         * 创建同样采样数的多采样 Renderbuffer。深度附件不用于
         * 当前 ImGui 显示，因此现阶段不需要 Resolve。
         */
        uint32_t m_DepthStencilRenderbuffer = 0;
    };
}
