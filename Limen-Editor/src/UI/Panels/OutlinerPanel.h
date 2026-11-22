#pragma once

#include "Limen/Core/Core.h"
#include "Limen/Scene/Scene.h"
#include "Limen/Tool/LArray.h"
#include "UI/EditorUILayout.h"

namespace Limen
{
    class Window;
    class FontAtlas;
}

namespace LimenEditor
{
    class EditorSession;
    class EditorUIRenderer;
    struct EditorTheme;

    enum class OutlinerPanelAction
    {
        None,
        SelectionChanged
    };

    // 场景对象面板：持有行布局和选择锚点。
    // 场景与选择数据仍归 EditorSession，面板只借用。
    class OutlinerPanel final
    {
    public:
        void Update(const EditorSession &session);

        void Draw(
            const EditorSession &session,
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &fontAtlas,
            const EditorTheme &theme,
            const UIRect &rect
        );

        // toggle/range 是调用方从 Input 读取的 Cmd/Ctrl 与 Shift 状态。
        OutlinerPanelAction OnMousePressed(
            EditorSession &session,
            float x,
            float y,
            bool toggle,
            bool range
        );

        void SetSelectionAnchor(Limen::SceneObjectID id) noexcept;

    private:
        UIRect m_Rect{};
        Limen::LArray<Limen::SceneObjectID> m_Rows;
        float m_FirstRowY = 0.0f;
        float m_RowHeight = 0.0f;
        Limen::SceneObjectID m_SelectionAnchor{};
    };
}
