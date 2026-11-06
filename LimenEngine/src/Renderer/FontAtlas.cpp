//
// Created by chenlong on 2026/11/3.
//

#include "Limen/Renderer/FontAtlas.h"

#include <fstream>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "Limen/Core/Log.h"
#include "Limen/RHI/Texture.h"
#include "Limen/Tool/LTuple.h"

namespace Limen
{
    namespace
    {
        // 解码一个 UTF-8 序列；非法序列按 U+FFFD 处理。
        LTuple<char32_t, std::size_t> DecodeUtf8(
            const std::string_view text, const std::size_t offset)
        {
            const unsigned char lead = static_cast<unsigned char>(text[offset]);
            if (lead < 0x80)
                return {static_cast<char32_t>(lead), std::size_t{1}};;

            std::size_t length = 0;
            char32_t codepoint = 0;
            if ((lead & 0xE0) == 0xC0)
            {
                length = 2;
                codepoint = lead & 0x1F;
            } else if ((lead & 0xF0) == 0xE0)
            {
                length = 3;
                codepoint = lead & 0x0F;
            } else if ((lead & 0xF8) == 0xF0)
            {
                length = 4;
                codepoint = lead & 0x07;
            } else
            {
                return {char32_t{0xFFFD}, std::size_t{1}};;
            }

            if (offset + length > text.size())
                return {char32_t{0xFFFD}, 1};

            for (std::size_t i = 1; i < length; ++i)
            {
                const unsigned char continuation =
                        static_cast<unsigned char>(text[offset + i]);
                if ((continuation & 0xC0) != 0x80)
                    return {0xFFFD, 1};
                codepoint = (codepoint << 6) | (continuation & 0x3F);
            }
            return {codepoint, length};
        }
    }

    FontAtlas::FontAtlas(const uint32_t atlasSize, const float fontPixelHeight)
        : m_AtlasSize(atlasSize), m_FontHeight(fontPixelHeight)
    {
    }

    bool FontAtlas::LoadFontFile(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            LM_CORE_ERROR("Don't exist this file {}", path.filename().string());
            return false;
        }

        // 调到文件结尾拿到字节数
        file.seekg(0, std::ios::end);
        const std::streamoff end = file.tellg();
        file.seekg(0, std::ios::beg);
        if (end <= 0)
        {
            LM_CORE_ERROR("{} is empty", path.filename().string());
        }
        // LArray 预分配后直接读进它的内存；之后 Build 一直借用这份字节
        m_FontBytes = LArray<std::uint8_t>(static_cast<uint32_t>(end));
        file.read(reinterpret_cast<char *>(m_FontBytes.getData()), end);

        // stbtt_InitFont 返回非0 表示文件是合法字体
        stbtt_fontinfo fontInfo{};
        return stbtt_InitFont(&fontInfo, m_FontBytes.getData(), 0) != 0;
    }

    void FontAtlas::AddText(const std::string_view utf8)
    {
        if (IsBuilt())
            return;
        for (std::size_t offset = 0; offset < utf8.size();)
        {
            const auto [codepoint, length] = DecodeUtf8(utf8, offset);
            if (!m_Glyphs.contains(codepoint))
                m_Glyphs.try_emplace(codepoint);
            offset += length;
        }
    }

    LArray<char32_t> FontAtlas::DecodeText(
    const std::string_view utf8) const
    {
        LArray<char32_t> codepoints;
        for (std::size_t offset = 0; offset < utf8.size();)
        {
            const auto [codepoint, length] = DecodeUtf8(utf8, offset);
            codepoints.push_back(codepoint);
            offset += length;
        }
        return codepoints;
    }

    float FontAtlas::GetAscent() const noexcept
    {
        return m_Ascent;
    }

    bool FontAtlas::Build()
{
    if (m_FontBytes.getSize() == 0 ||
        m_Glyphs.size() == 0 ||
        IsBuilt())
        return false;

    stbtt_fontinfo fontInfo{};
    if (!stbtt_InitFont(&fontInfo, m_FontBytes.getData(), 0))
        return false;

    // 度量：stb 返回字体单位，乘以缩放系数才是像素。
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    const float scale = stbtt_ScaleForPixelHeight(&fontInfo, m_FontHeight);
    m_Ascent = static_cast<float>(ascent) * scale;
    m_Descent = static_cast<float>(descent) * scale;
    m_LineGap = static_cast<float>(lineGap) * scale;

    // 码点是离散集合：每个码点一个 pack_range，chardata 指向自己的槽。
    LArray<stbtt_pack_range> ranges(static_cast<unsigned int>(m_Glyphs.size()));
    LArray<stbtt_packedchar> packed(static_cast<unsigned int>(m_Glyphs.size()));
    std::size_t index = 0;
    for (const auto &entry : m_Glyphs)
    {
        stbtt_pack_range range{};
        range.font_size = m_FontHeight;
        range.first_unicode_codepoint_in_range =
            static_cast<int>(entry.first);
        range.num_chars = 1;
        range.chardata_for_range = &packed[index];
        ranges[index] = range;
        ++index;
    }

    // 打包进图集；PackEnd 无论成败都要调用。
    LArray<unsigned char> pixels(m_AtlasSize * m_AtlasSize, 0);
    stbtt_pack_context context{};
    if (!stbtt_PackBegin(
            &context, pixels.getData(),
            static_cast<int>(m_AtlasSize), static_cast<int>(m_AtlasSize),
            0, 1, nullptr))
    {
        return false;
    }

    const bool packedOk =stbtt_PackFontRanges(&context, m_FontBytes.getData(), 0,
            ranges.getData(), static_cast<int>(ranges.getSize())) != 0;
    stbtt_PackEnd(&context);
    if (!packedOk)
        return false;

    // 灰度展开成 RGBA8（A 通道存灰度），纹理是数据纹理，走 Linear。
    LArray<std::uint8_t> rgba(
        static_cast<uint32_t>(pixels.getSize()) * 4, 0);
    for (uint32_t i = 0; i < pixels.getSize(); ++i)
    {
        rgba[i * 4 + 0] = 255;
        rgba[i * 4 + 1] = 255;
        rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = pixels[i];
    }

    Texture2DSpecification specification;
    specification.Width = m_AtlasSize;
    specification.Height = m_AtlasSize;
    specification.Format = TextureFormat::RGBA8;
    specification.ColorSpace = TextureColorSpace::Linear;
    m_AtlasTexture = Texture2D::Create(specification);
    if (!m_AtlasTexture)
        return false;
    m_AtlasTexture->SetData(rgba.getData(), rgba.getSize());

    // 回填 UV：SetData 第 0 行是纹理底行，顶点着色器已按左上原点映射，无需翻转。
    const float atlasSize = static_cast<float>(m_AtlasSize);
    index = 0;
    for (auto &entry : m_Glyphs)
    {
        const stbtt_packedchar &data = packed[index];
        auto &[
            U0,
            V0,
            U1,
            V1,
            OffsetX,
            OffsetY,
            Advance,
            Width,
            Height] = entry.second;
        U0 = static_cast<float>(data.x0) / atlasSize;
        U1 = static_cast<float>(data.x1) / atlasSize;
        V0 = static_cast<float>(data.y0) / atlasSize;
        V1 = static_cast<float>(data.y1) / atlasSize;
        OffsetX = data.xoff;
        OffsetY = data.yoff;
        Advance = data.xadvance;
        Width = static_cast<float>(data.x1 - data.x0);
        Height = static_cast<float>(data.y1 - data.y0);
        ++index;
    }
    return true;
}

    const FontAtlas::Glyph *FontAtlas::FindGlyph(const char32_t codepoint) const noexcept
    {
        const auto found = m_Glyphs.find(codepoint);
        return found == m_Glyphs.end() ? nullptr : &found->second;
    }

    const Texture2D &FontAtlas::GetTexture() const
    {
        return *m_AtlasTexture.get();
    }

    bool FontAtlas::IsBuilt() const noexcept
    {
        return m_AtlasTexture != nullptr;
    }

    float FontAtlas::GetFontHeight() const noexcept
    {
        return m_FontHeight;
    }

    float FontAtlas::GetLineHeight() const noexcept
    {
        return m_Ascent - m_Descent + m_LineGap;
    }
}
