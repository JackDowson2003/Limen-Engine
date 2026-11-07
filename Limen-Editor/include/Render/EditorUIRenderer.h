#pragma once

#include "Limen/Core/Core.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

namespace Limen
{
    class FontAtlas;
    class Window;
    class Texture2D;
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
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size,
            const glm::vec4 &color
        ) const;

        // 普通 UI 图片使用逻辑窗口坐标；texture 只在绘制调用期间借用。
        void DrawTexturedRect(
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size,
            const Limen::Texture2D &texture
        ) const;

        void DrawNativeTextureRect(const Limen::Window &window, const glm::vec2 &position,
                                   const glm::vec2 &size, std::uintptr_t textureHandle) const;

        void DrawGlyph(
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size,
            const glm::vec4 &color,
            const Limen::Texture2D &atlasTexture,
            const glm::vec2 &uvMin,
            const glm::vec2 &uvMax
        ) const;

        void SetClipRect(
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size
        ) const;

        // 从 position（行盒左上角）开始排版一行文字；未登记字形跳过。
        void DrawText(
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec4 &color,
            const Limen::FontAtlas &atlas,
            std::string_view text
        ) const;

    private:
        Limen::Scope<EditorUIRenderBackend> m_Backend;
    };
}
