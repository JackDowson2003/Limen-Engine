#include "Render/EditorUIRenderer.h"

#include "Render/EditorUIRenderBackend.h"
#include "Limen/Core/Log.h"
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
        const Limen::Window& window,
        const glm::vec2& position,
        const glm::vec2& size,
        const glm::vec4& color
    ) const
    {
        if (m_Backend)
            m_Backend->DrawSolidRect(window, position, size, color);
    }

    void EditorUIRenderer::DrawTexturedRect(const Limen::Window &window, const glm::vec2 &position,
        const glm::vec2 &size, const Limen::Texture2D &texture) const
    {
        if (m_Backend)
            m_Backend->DrawTexturedRect(window,position,size,texture);
    }
}
