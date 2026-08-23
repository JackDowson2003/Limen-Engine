//
// Created by chenlong on 2026/8/23.
//
#include "OpenGLFramebuffer.h"
#include <glad/gl.h>

#include "Limen/Core/Log.h"


namespace Limen
{
    OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification &specification)
        : m_Specification(specification)
    {
        Invalidate();
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        Release();
    }

    void OpenGLFramebuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

        // 离屏 Framebuffer 尺寸可能与应用窗口不同，因此绑定时同步设置视口。
        glViewport(0, 0,
                   static_cast<GLsizei>(m_Specification.Width),
                   static_cast<GLsizei>(m_Specification.Height));
    }

    void OpenGLFramebuffer::UnBind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Resolve() const
    {
        // 单采样直接渲染到可采样的颜色纹理，不需要 Resolve。
        if (m_Specification.Samples <= 1)
            return;

        /*
         * 保存调用者原本绑定的 Read / Draw Framebuffer，
         * 避免 Resolve 悄悄破坏外部 OpenGL 状态。
         */
        GLint previousReadFramebuffer = 0;
        GLint previousDrawFramebuffer = 0;

        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);

        // 从多采样场景 FBO 读取，写入持有普通 Texture2D 的 Resolve FBO。
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_RendererID);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolveFramebufferID);

        // 将完整颜色区域从多采样附件解析到单采样纹理。
        // 多采样 Resolve 的过滤参数必须使用 GL_NEAREST。
        glBlitFramebuffer(0,
                          0,
                          static_cast<GLint>(m_Specification.Width),
                          static_cast<GLint>(m_Specification.Height),

                          0,
                          0,
                          static_cast<GLint>(m_Specification.Width),
                          static_cast<GLint>(m_Specification.Height),

                          GL_COLOR_BUFFER_BIT,
                          GL_NEAREST
        );

        // 恢复调用 Resolve 前的 OpenGL 读写目标，避免污染调用方状态。
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFramebuffer));

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousDrawFramebuffer));
    }

    void OpenGLFramebuffer::Resize(const uint32_t width, const uint32_t height)
    {
        /*
         * Viewport 被最小化时可能出现 0x0。
         * 此时不能创建 OpenGL 附件，等待有效尺寸再 Resize。
         */
        if (width == 0 || height == 0)
            return;

        if (width == m_Specification.Width && height == m_Specification.Height)
        {
            return;
        }

        m_Specification.Width = width;
        m_Specification.Height = height;

        Invalidate();
    }

    void OpenGLFramebuffer::Invalidate()
    {
        // 删除旧附件，再按当前 Specification 重建。
        Release();
        /*
         * GL_MAX_SAMPLES 是当前 GPU 支持的最大 MSAA 样本数。
         * 若用户请求过高，安全地降低到硬件支持的最大值。
         */
        GLint maxSamples = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

        if (m_Specification.Samples > static_cast<uint32_t>(maxSamples))
        {
            LM_CORE_WARN("Requested {}x MSAA, but OpenGL supports at most {}x. Clamping.",
                         m_Specification.Samples,
                         maxSamples
            );
            m_Specification.Samples = static_cast<uint32_t>(maxSamples);
        }
        const int width = static_cast<int>(m_Specification.Width);

        const int height = static_cast<int>(m_Specification.Height);

        // m_ColorAttachment 始终是普通 Texture2D：单采样时直接接收场景颜色，
        // 多采样时接收 Resolve 结果，随后可交给 ImGui::Image 显示。
        glGenTextures(1, &m_ColorAttachment);

        // 场景是 2D 还是 3D 不影响渲染目标的维度；屏幕颜色结果仍是二维纹理。
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);

        // 渲染目标不生成 Mipmap，因此缩小过滤不能选择 Mipmap 模式。
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

        // 只分配 GPU 存储；Framebuffer 渲染阶段才会写入像素。
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            static_cast<GLint>(GL_RGBA8),
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            nullptr
        );


        glBindTexture(GL_TEXTURE_2D, 0);

        // m_RendererID 是场景绘制时绑定的主 FBO。
        glGenFramebuffers(1, &m_RendererID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

        if (m_Specification.Samples == 1)
        {
            // 单采样：普通颜色纹理直接挂载到主 FBO。
            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                m_ColorAttachment,
                0
            );

            glGenRenderbuffers(1, &m_DepthStencilRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRenderbuffer);

            // 分配 24 位深度和 8 位模板存储，稍后统一挂载到主 FBO。
            glRenderbufferStorage(GL_RENDERBUFFER,
                                  GL_DEPTH24_STENCIL8,
                                  width, height);
        } else
        {
            // 多采样：颜色与深度/模板附件必须使用相同的样本数。
            glGenRenderbuffers(1, &m_MultisampleColorRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, m_MultisampleColorRenderbuffer);

            // 多采样颜色先存入 Renderbuffer，之后 Resolve 到 m_ColorAttachment。
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                                             static_cast<GLsizei>(m_Specification.Samples),
                                             GL_RGBA8,
                                             width, height
            );

            // 把多采样颜色 Renderbuffer 挂到场景 FBO 的颜色附件 0。
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                      GL_COLOR_ATTACHMENT0,
                                      GL_RENDERBUFFER,
                                      m_MultisampleColorRenderbuffer);

            // 创建采样数一致的深度/模板 Renderbuffer。
            glGenRenderbuffers(1, &m_DepthStencilRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRenderbuffer);

            // 为每个样本分配深度/模板存储。
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                                             static_cast<GLsizei>(m_Specification.Samples),
                                             GL_DEPTH24_STENCIL8,
                                             width, height);
        }

        // 无论是否启用 MSAA，都把深度/模板 Renderbuffer 挂到主 FBO。
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                  GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER,
                                  m_DepthStencilRenderbuffer
        );
        LM_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                       "OpenGL scene Framebuffer is incomplete");

        if (m_Specification.Samples > 1)
        {
            glGenFramebuffers(1, &m_ResolveFramebufferID);
            glBindFramebuffer(GL_FRAMEBUFFER, m_ResolveFramebufferID);

            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D,
                                   m_ColorAttachment,
                                   0
            );
            LM_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                           "OpenGL resolve Framebuffer is incomplete"
            );
        }

        // 恢复默认绑定，避免后续资源操作误改本 Framebuffer。
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Release()
    {
        // 先解绑待删除对象，避免 Resize 后继续引用旧附件。
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (m_DepthStencilRenderbuffer != 0)
        {
            glDeleteRenderbuffers(1, &m_DepthStencilRenderbuffer);
            m_DepthStencilRenderbuffer = 0;
        }
        if (m_MultisampleColorRenderbuffer != 0)
        {
            glDeleteRenderbuffers(1, &m_MultisampleColorRenderbuffer);
            m_MultisampleColorRenderbuffer = 0;
        }
        if (m_ColorAttachment != 0)
        {
            glDeleteTextures(1, &m_ColorAttachment);
            m_ColorAttachment = 0;
        }
        if (m_ResolveFramebufferID != 0)
        {
            glDeleteFramebuffers(1, &m_ResolveFramebufferID);
            m_ResolveFramebufferID = 0;
        }
        if (m_RendererID != 0)
        {
            glDeleteFramebuffers(1, &m_RendererID);
            m_RendererID = 0;
        }
    }
}
