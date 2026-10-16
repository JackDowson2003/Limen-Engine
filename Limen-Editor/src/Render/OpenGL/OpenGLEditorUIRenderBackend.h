#pragma once

#include "Render/EditorUIRenderBackend.h"
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
            const Limen::Window& window,
            const glm::vec2& position,
            const glm::vec2& size,
            const glm::vec4& color
        ) override;

    private:
        Limen::Scope<Limen::VertexArray> m_VertexArray;
        Limen::Ref<Limen::GraphicsPipeline> m_Pipeline;
    };
}
