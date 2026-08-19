//
// Created by chenlong on 2026/8/19.
//

#pragma once
#include "Renderer/Texture.h"

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
        //BPP是每个像素所需位数 BPP = 颜色通道数 × 每个通道的位数 和我们选择的格式有关
        //还有个BPB Bytes Per Pixel
        uint32_t m_Width, m_Height, m_BPP;
        std::string m_Path;
    };

}
