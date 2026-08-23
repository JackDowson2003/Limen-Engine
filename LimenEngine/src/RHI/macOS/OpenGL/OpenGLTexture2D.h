//
// Created by chenlong on 2026/8/19.
//

#pragma once
#include "Limen/RHI/Texture.h"

namespace Limen
{
    class OpenGLTexture2D : public Texture2D
    {
    public:
        explicit  OpenGLTexture2D(const std::string & path);
        ~OpenGLTexture2D() override;
        
         uint32_t GetWidth() const noexcept override
         {
             return m_Width;
         }

         uint32_t GetHeight() const noexcept override
         {
             return m_Height;
         }

         void Bind(uint32_t slot = 0) const override;
        
        OpenGLTexture2D(const OpenGLTexture2D& other) = delete;
        OpenGLTexture2D& operator=(const OpenGLTexture2D& other) = delete;

        OpenGLTexture2D(OpenGLTexture2D&& other);
        OpenGLTexture2D& operator=(OpenGLTexture2D&& other);

        static Ref<OpenGLTexture2D> LoadFromFile(const char* path);

    private:
        uint32_t m_RendererID;

        // m_BPP 保存源图像的通道数（RGB 为 3，RGBA 为 4）。
        uint32_t m_Width, m_Height, m_BPP;
        std::string m_Path;
    };

}
