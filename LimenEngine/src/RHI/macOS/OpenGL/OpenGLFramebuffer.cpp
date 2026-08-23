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

        /**
         * 窗口尺寸可能和 Framebuffer不同
         * 例如窗口是 1600x900，但 ImGui Scene Viewport
         * 只有 1000x600，此时必须使用 Framebuffer 自己的 Viewport。
         */
        glViewport(0, 0,
                   static_cast<GLsizei>(m_Specification.Width),
                   static_cast<GLsizei>(m_Specification.Height)); //重置viewport
    }

    void OpenGLFramebuffer::UnBind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Resolve() const
    {
        /**
         * 单采样的时候 就是直接画入，就不需要resolve
         */
        if (m_Specification.Samples <= 1)
            return;

        /*
         * 保存调用者原本绑定的 Read / Draw Framebuffer，
         * 避免 Resolve 悄悄破坏外部 OpenGL 状态。
         */
        GLint previousReadFramebuffer = 0;
        GLint previousDrawFramebuffer = 0;

        //向OpenGL驱动查询各种状态参数，把读到的值存到int数组里
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);

        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);

        //从 m_RendererID 读取
        // 向 m_ResolveFramebufferID 写入
        // 从多采样 FBO 读。
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_RendererID); //read
        // 向拥有普通 Texture2D 的 Resolve FBO 写。
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolveFramebufferID); //draw

        /**
         * 前四个参数 表示从源FBO的整个画面读取
         * 中间四个表示写满目标FBO
         * GL_COLOR_BUFFER_BIT 表示只处理颜色
         * 这里不是给纹理选择“最近邻过滤效果”
         * 而是 glBlitFramebuffer 进行 MSAA Resolve 时要求使用的过滤参数。
         *
         * 这里创建了 m_ColorAttachment 每个像素保存 RGBA 颜色
         * 但它不保存这个像素距离相机多远。
         *
         * 多采样 Resolve 必须使用 GL_NEAREST
         *
         * 这里的“最近”不是纹理过滤；它让 OpenGL 按样本规则
         * 把多采样结果 Resolve 到单采样颜色纹理。
         */
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

        // 恢复调用 Resolve 前的状态。
        //因为前面用read向m_RendererID读取了数据了的
        //DRAW又向 Resolve FBO 写了的
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

    /**
     * 删除旧附件，再按当前 Specification 重建。
     */
    void OpenGLFramebuffer::Invalidate()
    {
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

        /*
        * m_ColorAttachment 永远是普通 Texture2D。
        *
        * - 无 MSAA：它直接作为场景颜色附件。
        * - 有 MSAA：它作为 Resolve 的目标，最后交给 ImGui。
        */
        glGenTextures(1, &m_ColorAttachment);

        /**
         *不需要因为“3D 模型”而改成 GL_TEXTURE_3D。
         * GL_TEXTURE_2D 描述的是纹理/渲染结果本身的维度，不是场景的维度：
         *
         */
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);

        //不生成 MipMap，所以不能使用 GL_LINEAR_MIPMAP_LINEAR
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

        //给2D纹理开启空间
        glTexImage2D(
            GL_TEXTURE_2D, // 当前绑定的2D纹理
            0, // 上传Level 0，也就是原始最高分辨率图片
            static_cast<GLint>(GL_RGBA8), // GPU内部存储格式
            width,
            height,
            0, // 必须是0
            GL_RGBA, // CPU数据格式
            GL_UNSIGNED_BYTE, // 每个通道的数据类型
            nullptr
        );


        glBindTexture(GL_TEXTURE_2D, 0);

        //场景真正的绘制
        glGenFramebuffers(1, &m_RendererID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

        if (m_Specification.Samples == 1) //单采样
        {
            //单采样：颜色纹理直接作为场景颜色附件。
            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                m_ColorAttachment,
                0
            );

            glGenRenderbuffers(1, &m_DepthStencilRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRenderbuffer);

            /**
             * 给RenderBuffer分配显存空间，只进行分配，还没有挂载到FBO
             * GL_DEPTH24_STENCIL8
             * • Depth深度占 24bit
             * • Stencil模板占 8bit
             *  GL_DEPTH_COMPONENT24：只存深度24位，不要模板
             */
            glRenderbufferStorage(GL_RENDERBUFFER,
                                  GL_DEPTH24_STENCIL8,
                                  width, height);
        } else
        {
            //多采样，颜色和深度都是采样附件
            glGenRenderbuffers(1, &m_MultisampleColorRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, m_MultisampleColorRenderbuffer);

            /**
             * 创建 MSAA 颜色空间
             * m_DepthStencilRenderbuffer
               ├── 每个像素保存 24 位深度
               └── 每个像素保存 8 位模板
             **/
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

            //创建 MSAA 深度/模板空间
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                                             static_cast<GLsizei>(m_Specification.Samples),
                                             GL_DEPTH24_STENCIL8,
                                             width, height);
        }

        //无论是否MSAA 3D，都需要这张深度/模版附件
        //深度挂载上去
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

        //Renderbuffer 的解绑主要是为了保持状态干净、防止误修改
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        //FBO 的解绑更重要，因为它直接决定后续 Draw 画进离屏纹理还是窗口
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Release()
    {
        //Resize旧的时候 FBO可能扔绑定，先解绑更安全
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
