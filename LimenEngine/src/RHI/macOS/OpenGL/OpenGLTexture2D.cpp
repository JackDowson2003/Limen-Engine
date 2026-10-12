//
// Created by chenlong on 2026/8/19.
//

#include "OpenGLTexture2D.h"

#include "Limen/Core/Log.h"
#include <glad/gl.h>

namespace Limen
{

    OpenGLTexture2D::OpenGLTexture2D(const Texture2DSpecification &specification)
        : m_Width(specification.Width),
          m_Height(specification.Height),
          m_Format(specification.Format),
          m_GenerateMipmaps(specification.GenerateMipmaps)
    {
        LM_CORE_ASSERT(
            m_Width > 0 && m_Height > 0,
            "Texture dimensions must be greater than zero"
        );

        uint32_t internalFormat = 0;
        uint32_t dataFormat = 0;
        const bool useSRGB = specification.ColorSpace == TextureColorSpace::SRGB;

        switch (m_Format)
        {
            case TextureFormat::RGB8:
            {
                internalFormat = useSRGB ? GL_SRGB8 :  GL_RGB8;
                dataFormat = GL_RGB;
                m_BPP = 3;
                break;
            }
            case TextureFormat::RGBA8:
            {
                internalFormat = useSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
                dataFormat = GL_RGBA;
                m_BPP = 4;
                break;
            }
            default:
                LM_CORE_ASSERT(false, "Unsupported texture format");
                return;
        }

        glGenTextures(1, &m_RendererID);
        OpenGLTexture2D::Bind();

        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_MIN_FILTER,
                        m_GenerateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR
        );

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

        // 申请 GPU 空间 暂时不上传像素，因此最后一个参数是nullptr
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            static_cast<GLint>(internalFormat),
            static_cast<GLsizei>(m_Width),
            static_cast<GLsizei>(m_Height),
            0,
            dataFormat,
            GL_UNSIGNED_BYTE,
            nullptr
        );

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void OpenGLTexture2D::SetData(const void *data, const uint32_t size)
    {
        LM_CORE_ASSERT(data, "Texture data cannot be null");

        // 一个完整的纹理所需要的字节数
        const uint64_t expectedSize =
                static_cast<uint64_t>(m_Width) * static_cast<uint64_t>(m_Height) * m_BPP;

        LM_CORE_ASSERT(
            size == expectedSize,
            "Texture data size does not match texture specification"
        );

        if (!data || size != expectedSize)
            return;

        GLenum dataFormat = 0;

        switch (m_Format)
        {
            case TextureFormat::RGB8:
                dataFormat = GL_RGB;
                break;

            case TextureFormat::RGBA8:
                dataFormat = GL_RGBA;
                break;

            default:
                LM_CORE_ASSERT(false, "Unsupported texture format");
                return;
        }
        OpenGLTexture2D::Bind();

        /*
         * OpenGL默认要求每行像素按4字节对齐。
         * RGB纹理的某些宽度不满足这个要求，因此上传期间改为1字节对齐。
         */
        GLint previousUnpackAlignment = 0;

        glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // 更新已经由glTexImage2D申请好的Level 0纹理空间。
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            static_cast<GLsizei>(m_Width),
            static_cast<GLsizei>(m_Height),
            dataFormat,
            GL_UNSIGNED_BYTE,
            data
        );

        glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

        if (m_GenerateMipmaps)
            glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    OpenGLTexture2D::~OpenGLTexture2D()
    {
        glDeleteTextures(1, &m_RendererID);
    }

    void OpenGLTexture2D::Bind(const uint32_t slot) const
    {
        // 激活调用方指定的纹理单元，再把该纹理绑定到该单元。
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
    }

    OpenGLTexture2D::OpenGLTexture2D(OpenGLTexture2D &&other) noexcept
        : m_RendererID(other.m_RendererID),
          m_Width(other.m_Width),
          m_Height(other.m_Height),
          m_BPP(other.m_BPP),
          m_Format(other.m_Format),
          m_GenerateMipmaps(other.m_GenerateMipmaps)
    {
        /*
         * GPU纹理所有权已经转移给当前对象。
         * 将源对象置为空，防止其析构时重复删除纹理。
         */
        other.m_RendererID = 0;
        other.m_Width = 0;
        other.m_Height = 0;
        other.m_BPP = 0;
        other.m_Format = TextureFormat::None;
        other.m_GenerateMipmaps = false;
    }

    OpenGLTexture2D &OpenGLTexture2D::operator=(OpenGLTexture2D &&other) noexcept
    {
        if (this == &other)
            return *this;

        /*
         * 当前对象可能已经拥有一个GPU纹理。
         * 覆盖RendererID之前必须先释放，否则旧纹理会泄漏。
         */
        if (m_RendererID != 0)
            glDeleteTextures(1, &m_RendererID);

        m_RendererID = other.m_RendererID;
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_BPP = other.m_BPP;
        m_Format = other.m_Format;
        m_GenerateMipmaps = other.m_GenerateMipmaps;

        // 清空源对象，防止重复释放同一个GPU纹理。
        other.m_RendererID = 0;
        other.m_Width = 0;
        other.m_Height = 0;
        other.m_BPP = 0;
        other.m_Format = TextureFormat::None;
        other.m_GenerateMipmaps = false;

        return *this;
    }
}
