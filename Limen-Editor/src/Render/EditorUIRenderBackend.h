#pragma once

#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

namespace Limen
{
    class Window;
}

namespace LimenEditor
{
    // Editor 私有协议：具体图形后端仅实现操作，不决定后端选择。
    class EditorUIRenderBackend
    {
    public:
        virtual ~EditorUIRenderBackend() = default;

        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual void DrawSolidRect(
            const Limen::Window& window,
            const glm::vec2& position,
            const glm::vec2& size,
            const glm::vec4& color
        ) = 0;
    };

}
