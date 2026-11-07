#pragma once

#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace Limen
{
    class Window;
    class Texture2D;
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
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size,
            const glm::vec4 &color
        ) = 0;

        virtual void DrawTexturedRect(
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size,
            const Limen::Texture2D &texture
        ) = 0;

        // 绘制已经是显示编码的纹理：不做任何颜色空间转换，直接采样输出。
        // textureHandle 是当前图形后端识别的原生纹理标识。
        virtual void DrawNativeTextureRect(
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size,
            std::uintptr_t textureHandle
        ) = 0;

        // 用字形图集画一个字形；position 是左上角逻辑坐标，颜色用于调制。
        virtual void DrawGlyph(
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size,
            const glm::vec4 &color,
            const Limen::Texture2D &atlasTexture,
            const glm::vec2 &uvMin,
            const glm::vec2 &uvMax
        ) = 0;

        // 后续绘制只落在该逻辑矩形内；宽或高不大于 0 表示取消裁剪。
        virtual void SetClipRect(
            const Limen::Window &window,
            const glm::vec2 &position,
            const glm::vec2 &size
        ) = 0;
    };
}
