//
// Created by chenlong on 2026/11/21.
//

#pragma once

#include <string>

#include "Limen/Tool/LStaticArray.h"
#include "UI/EditorUILayout.h"
#include "UI/Panels/DetailsTransformInput.h"

namespace Limen
{
    class Window;
    class FontAtlas;
    enum class KeyCode : std::uint16_t;
}

namespace LimenEditor
{
    class EditorSession;
    class EditorUIRenderer;
    struct EditorTheme;

    // Details 借用会话读写选中对象，仅保存文本草稿与绘制命中区域。
    class DetailsPanel final
    {
    public:
        void Draw(
            const EditorSession &session,
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &fontAtlas,
            const EditorTheme &theme,
            const UIRect &rect
        );

        [[nodiscard]] bool OnMousePressed(EditorSession &session, float x, float y);
        [[nodiscard]] bool OnKeyPressed(EditorSession &session, Limen::KeyCode key);
        [[nodiscard]] bool OnKeyTyped(char32_t codepoint);
        [[nodiscard]] bool IsEditing() const noexcept { return m_EditingObject.isValid(); }
        [[nodiscard]] bool CommitEdit(EditorSession &session);
        void ResetHitAreas() noexcept;

    private:
        void CancelEdit();
        Limen::LStaticArray<UIRect, 3> m_TransformFields{};
        Limen::SceneObjectID m_PaintedObject{};
        Limen::SceneObjectID m_EditingObject{};
        DetailsTransformField m_Field = DetailsTransformField::Location;
        std::string m_Draft;
        bool m_DraftSelected = false;
        bool m_InvalidDraft = false;
    };
}
