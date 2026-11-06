#pragma once

#include "../../../include/Render/EditorUIRenderBackend.h"
#include "Limen/Core/Core.h"

namespace Limen
{
    class GraphicsPipeline;
    class VertexArray;
}

namespace LimenEditor
{
    // 仅由 Editor 的 OpenGL 构建配置编译和实例化。
    class OpenGLEditorUIRenderBackend final : public EditorUIRenderBackend
    {
    public:
        ~OpenGLEditorUIRenderBackend() override;

        bool Initialize() override;

        void Shutdown() override;

        void DrawSolidRect(
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size,
            const glm::vec4 &color
        ) override;

        void DrawTexturedRect(
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size,
            const Limen::Texture2D &texture
        ) override;

        void DrawNativeTextureRect(
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size,
            std::uintptr_t textureHandle
        ) override;
        void DrawGlyph(
            const Limen::Window& window,
            const glm::vec2& position,
            const glm::vec2& size,
            const glm::vec4& color,
            const Limen::Texture2D& atlasTexture,
            const glm::vec2& uvMin,
            const glm::vec2& uvMax
        )  override;
    private:
        Limen::Scope<Limen::VertexArray> m_VertexArray;

        Limen::Ref<Limen::GraphicsPipeline> m_TexturedPipeline;

        Limen::Ref<Limen::GraphicsPipeline> m_Pipeline;

        Limen::Ref<Limen::GraphicsPipeline> m_NativePipeline;

        Limen::Ref<Limen::GraphicsPipeline> m_GlyphPipeline;
    };
}
