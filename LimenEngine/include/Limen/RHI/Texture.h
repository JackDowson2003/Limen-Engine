//
// Created by chenlong on 2026/8/11.
//

#pragma once

#include "Limen/Core/Core.h"

namespace Limen
{
    // 纹理像素在 GPU 中的存储格式
    enum class TextureFormat : uint8_t
    {
        None = 0,

        // 每个像素包含红、绿、蓝三个8位通道。
        RGB8,

        // 每个像素包含红、绿、蓝、透明度四个8位通道。
        RGBA8
    };

    /**
     * @brief 创建一张空Texture2D时需要的参数。
     */
    struct Texture2DSpecification
    {
        // 纹理宽度，单位为像素。
        uint32_t Width = 1;

        // 纹理高度，单位为像素。
        uint32_t Height = 1;

        // 默认使用RGBA8，能够同时支持颜色和透明度。
        TextureFormat Format = TextureFormat::RGBA8;

        // 是否为纹理生成Mipmap。
        bool GenerateMipmaps = false;
    };

    class LIMEN_API Texture
    {
    public:
        virtual ~Texture() = default;

        virtual uint32_t GetWidth() const noexcept = 0;

        virtual uint32_t GetHeight() const noexcept = 0;

        virtual void Bind(uint32_t slot = 0) const = 0;
    };

    class LIMEN_API Texture2D : public Texture
    {
    public:
        /**
         * @brief 从图片文件加载纹理。
         */
        static Ref<Texture2D> Create(const char* path);

        /**
         * @brief 根据规格创建一张没有初始像素内容的纹理。
         *
         * 后续通过SetData上传像素数据。
         */
        static Ref<Texture2D> Create(
            const Texture2DSpecification& specification
        );

        /**
         * @brief 将完整的像素数据上传到纹理。
         *
         * @param data 指向CPU端像素数据。
         * @param size 数据总字节数。
         */
        virtual void SetData(const void* data, uint32_t size) = 0;
    };
}
