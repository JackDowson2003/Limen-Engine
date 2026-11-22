//
// Created by chenlong on 2026/11/21.
//

#pragma once

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

    // Details 只读取会话选择状态，不拥有 Scene 或 GPU 资源。
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
        ) const;
    };
}
