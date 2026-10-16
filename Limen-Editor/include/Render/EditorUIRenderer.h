#pragma once

#include "Limen/Core/Core.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

namespace Limen
{
    class Window;
}

namespace LimenEditor
{
    class EditorUIRenderBackend;

    class EditorUIRenderer
    {
    public:
        EditorUIRenderer();
        ~EditorUIRenderer();

        bool Initialize();
        void Shutdown();

        // position、size 使用左上角为原点的逻辑窗口坐标。
        void DrawSolidRect(
            const Limen::Window& window,
            const glm::vec2& position,
            const glm::vec2& size,
            const glm::vec4& color
        ) const;

    private:
        Limen::Scope<EditorUIRenderBackend> m_Backend;
    };
}