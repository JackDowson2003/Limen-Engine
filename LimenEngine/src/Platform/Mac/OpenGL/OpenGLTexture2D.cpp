//
// Created by chenlong on 2026/8/19.
//

#include "OpenGLTexture2D.h"

#include "Log.h"
#include "stb_image/stb_image.h"
#include <glad/gl.h>

namespace Limen
{
    OpenGLTexture2D::OpenGLTexture2D(const std::string &path)
        : m_Path(path)
    {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(1); //垂直翻转
        stbi_uc *data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        LM_CORE_ASSERT(data, "Failed to load image!");
        if (!data)
        {
            LM_CORE_ERROR(
                "Failed to load texture '{}': {}",
                path,
                stbi_failure_reason()
            );
            return;
        }
        m_Width = static_cast<uint32_t>(width);
        m_Height = static_cast<uint32_t>(height);
        m_BPP = static_cast<uint32_t>(channels);

        glGenTextures(1, &m_RendererID);
        OpenGLTexture2D::Bind(); //必须先Bind，只是为了初始化纹理，不表示这张纹理永远占用槽位 0

        GLenum internalFormat = 0, dataFormat = 0;
        if (channels == 4)
        {
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
        }
        else if (channels == 3)
        {
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
        }
        LM_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");

        // 当纹理距离相机太远（纹理 > 屏幕区域）时使用线性插值 采用双线性过滤
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        // glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        // glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR));
        //当纹素占据多个像素的时候就采用此算法 不过会有锯齿/马赛克感，因为离相机太近了 采用双线性过滤
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        // S (u) 方向（水平）超出时重复GL_REPEAT GL_CLAMP_TO_EDGE 拉伸 GL_MIRRORED_REPEAT 镜像重复   GL_CLAMP_TO_BORDER 显示边框颜色
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        // T (v) 方向（水平）超出时重复
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

        glTexImage2D(
            GL_TEXTURE_2D, // 当前绑定的2D纹理
            0, // 上传Level 0，也就是原始最高分辨率图片
            static_cast<GLint>(internalFormat), // GPU内部存储格式
            width,
            height,
            0, // 必须是0
            dataFormat, // CPU数据格式
            GL_UNSIGNED_BYTE, // 每个通道的数据类型
            data // stbi_load返回的数据
        );

        glGenerateMipmap(GL_TEXTURE_2D); //生成其余层

        glBindTexture(GL_TEXTURE_2D, 0);
        if (data)
            stbi_image_free(data);
    }

    OpenGLTexture2D::~OpenGLTexture2D()
    {
        glDeleteTextures(1, &m_RendererID);
    }

    void OpenGLTexture2D::Bind(const uint32_t slot) const
    {
        glActiveTexture(GL_TEXTURE0 + slot); //源码是按顺序来的 总共GL_TEXTURE31 0~31
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
    }

    OpenGLTexture2D::OpenGLTexture2D(OpenGLTexture2D &&other)
    {
        if (this == &other)return;
        m_RendererID = other.m_RendererID;
        m_Height = other.m_Height;
        m_Width = other.m_Width;
        m_Path = other.m_Path;
        other.m_RendererID = 0;
        other.m_Width = 0;
        other.m_Height = 0;
        other.m_Path = nullptr;
    }

    OpenGLTexture2D &OpenGLTexture2D::operator=(OpenGLTexture2D &&other)
    {
        if (this == &other)
            return *this;
        m_RendererID = other.m_RendererID;
        m_Height = other.m_Height;
        m_Width = other.m_Width;
        m_Path = other.m_Path;
        other.m_RendererID = 0;
        other.m_Width = 0;
        other.m_Height = 0;
        other.m_Path = nullptr;

        return *this;
    }

    Ref<OpenGLTexture2D> OpenGLTexture2D::LoadFromFile(const char *path)
    {
        return CreateRef<OpenGLTexture2D>(path);
    }
}
