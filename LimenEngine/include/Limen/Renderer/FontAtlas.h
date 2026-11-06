//
// Created by chenlong on 2026/11/3.
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

#include "Limen/Core/Core.h"
#include "Limen/Tool/LArray.h"
#include "Limen/Tool/LMap.h"

namespace Limen
{
    class Texture2D;

    // 只负责 字体的字节流 -> 字形度量与图集纹理，不处理排班与断行
    // 引擎自带默认字体，Editor 等客户端通过资产根目录取得字体路径
    class LIMEN_API FontAtlas final
    {
    public:
        // 图集内单个字形的布局；UV 为图集归一化坐标，其余单位为像素。
        struct Glyph
        {
            float U0 = 0.0f;
            float V0 = 0.0f;
            float U1 = 0.0f;
            float V1 = 0.0f;
            float OffsetX = 0.0f;   // 相对笔位起点的水平偏移
            float OffsetY = 0.0f;   // 相对基线的垂直偏移
            float Advance = 0.0f;   // 笔位前进量

            float Width = 0.0f;    // 字形位图像素宽
            float Height = 0.0f;   // 字形位图像素高
        };

        explicit FontAtlas(uint32_t atlasSize = 1024, float fontPixelHeight = 32.0f);
        ~FontAtlas() = default;

        // Cannot copy this class
        FontAtlas(const FontAtlas&) =delete;
        FontAtlas& operator=(const FontAtlas&) = delete;

        // 只读入字体文件字节并校验，必须在Build前调用
        bool LoadFontFile(const std::filesystem::path& path);

        // 登记文本需要的码点；可多次调用，Build 后调用无效。
        void AddText(std::string_view utf8);

        // 栅格化全部已登记码点并上传图集纹理；已 Build 后再次调用返回 false。
        bool Build();

        [[nodiscard]] const Glyph *FindGlyph(char32_t codepoint) const noexcept;
        [[nodiscard]] const Texture2D &GetTexture() const;   // Build 之后借用
        [[nodiscard]] bool IsBuilt() const noexcept; // 是否构建
        [[nodiscard]] float GetFontHeight() const noexcept;   // 像素
        [[nodiscard]] float GetLineHeight() const noexcept;   // 像素
        // 把 UTF-8 文本解码成码点序列；非法序列落为 U+FFFD。
        [[nodiscard]] LArray<char32_t> DecodeText(std::string_view utf8) const;
        [[nodiscard]] float GetAscent() const noexcept;   // 像素，Build 后有效

    private:
        uint32_t m_AtlasSize = 1024;
        float m_FontHeight = 32.0f;
        LArray<std::uint8_t> m_FontBytes;
        LMap<char32_t, Glyph> m_Glyphs;
        Ref<Texture2D> m_AtlasTexture;
        float m_Ascent = 0.0f;
        float m_Descent = 0.0f;
        float m_LineGap = 0.0f;
    };

}

