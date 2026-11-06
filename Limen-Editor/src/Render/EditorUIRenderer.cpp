#include "Render/EditorUIRenderer.h"

#include "Render/EditorUIRenderBackend.h"
#include "Limen/Core/Log.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Limen/RHI/RendererAPI.h"

#if defined(LIMEN_EDITOR_WITH_OPENGL)
#include "Render/OpenGL/OpenGLEditorUIRenderBackend.h"
#endif

#if defined(LIMEN_EDITOR_WITH_DX12)
#include "Render/DX12/DX12EditorUIRenderBackend.h"
#endif

namespace LimenEditor
{
    EditorUIRenderer::EditorUIRenderer() = default;

    EditorUIRenderer::~EditorUIRenderer()
    {
        Shutdown();
    }

    bool EditorUIRenderer::Initialize()
    {
        Shutdown();

        switch (Limen::RendererAPI::GetAPI())
        {
            case Limen::RendererAPI::API::OPENGL:
#if defined(LIMEN_EDITOR_WITH_OPENGL)
                m_Backend = Limen::CreateScope<OpenGLEditorUIRenderBackend>();
                break;
#else
                LM_ERROR("Editor UI: OpenGL backend is not built for this platform");
                return false;
#endif
            case Limen::RendererAPI::API::DIRECT12:
#if defined(LIMEN_EDITOR_WITH_DX12)
                m_Backend = Limen::CreateScope<DX12EditorUIRenderBackend>();
                break;
#else
                LM_ERROR("Editor UI: Direct3D 12 backend is not implemented");
                return false;
#endif
            default:
                LM_ERROR("Editor UI: selected renderer backend is not implemented");
                return false;
        }

        if (!m_Backend || !m_Backend->Initialize())
        {
            LM_ERROR("Editor UI: backend initialization failed");
            Shutdown();
            return false;
        }
        return true;
    }

    void EditorUIRenderer::Shutdown()
    {
        if (!m_Backend)
            return;

        m_Backend->Shutdown();
        m_Backend.reset();
    }

    void EditorUIRenderer::DrawSolidRect(
        const Limen::Window &window,
        const glm::vec2 &position,
        const glm::vec2 &size,
        const glm::vec4 &color
    ) const
    {
        if (m_Backend)
            m_Backend->DrawSolidRect(window, position, size, color);
    }

    void EditorUIRenderer::DrawTexturedRect(const Limen::Window &window, const glm::vec2 &position,
                                            const glm::vec2 &size, const Limen::Texture2D &texture) const
    {
        if (m_Backend)
            m_Backend->DrawTexturedRect(window, position, size, texture);
    }

    void EditorUIRenderer::DrawNativeTextureRect(const Limen::Window &window, const glm::vec2 &position,
                                                 const glm::vec2 &size, std::uintptr_t textureHandle) const
    {
        if (m_Backend)
            m_Backend->DrawNativeTextureRect(window, position, size, textureHandle);
    }

    void EditorUIRenderer::DrawGlyph(const Limen::Window &window, const glm::vec2 &position, const glm::vec2 &size,
                                     const glm::vec4 &color, const Limen::Texture2D &atlasTexture,
                                     const glm::vec2 &uvMin,
                                     const glm::vec2 &uvMax) const
    {
        if (m_Backend)
            m_Backend->DrawGlyph(window, position, size, color, atlasTexture, uvMin, uvMax);
    }

    void EditorUIRenderer::DrawText(
        const Limen::Window &window,
        const glm::vec2 &position,
        const glm::vec4 &color,
        const Limen::FontAtlas &atlas,
        const std::string_view text) const
    {
        if (!m_Backend || !atlas.IsBuilt())
            return;

        // position 是行盒左上角；基线在顶部之下 ascent 处。
        const float baselineY = position.y + atlas.GetAscent();
        float penX = position.x;
        for (const char32_t codepoint: atlas.DecodeText(text))
        {
            const Limen::FontAtlas::Glyph *glyph = atlas.FindGlyph(codepoint);
            if (!glyph)
            {
                // 图集没有的字形：只前进一段空隙，避免文字粘连。
                penX += atlas.GetFontHeight() * 0.25f;
                continue;
            }

            // 空格等无位图字形：只前进笔位，不绘制、不报错。
            if (glyph->Width <= 0.0f || glyph->Height <= 0.0f)
            {
                penX += glyph->Advance;
                continue;
            }

            const glm::vec2 glyphTopLeft(
                penX + glyph->OffsetX,
                baselineY - glyph->OffsetY);
            m_Backend->DrawGlyph(
                window,
                glyphTopLeft,
                {glyph->Width, glyph->Height},
                color,
                atlas.GetTexture(),
                {glyph->U0, glyph->V0},
                {glyph->U1, glyph->V1});

            penX += glyph->Advance;
        }
    }
}
