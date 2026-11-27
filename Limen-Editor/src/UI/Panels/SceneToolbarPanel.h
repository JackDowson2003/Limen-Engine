#pragma once

#include "Limen/Tool/LStaticArray.h"
#include "UI/EditorUILayout.h"

namespace Limen
{
    class FontAtlas;
    class Window;
}

namespace LimenEditor
{
    class EditorUIRenderer;
    struct EditorTheme;

    enum class SceneToolbarAction
    {
        None,
        New,
        Open,
        Save,
        SaveAs
    };

    // 顶栏场景按钮只发出操作意图，场景和文件由调用方管理。
    class SceneToolbarPanel final
    {
    public:
        void Draw(
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &fontAtlas,
            const EditorTheme &theme,
            const UIRect &rect);

        [[nodiscard]] SceneToolbarAction HitTest(float x, float y) const noexcept;

        // 窗口尺寸变化时使上一帧的逻辑坐标失效。
        void Reset() noexcept;

    private:
        struct Button
        {
            SceneToolbarAction Action;
            const char *Label;
            float Width;
            UIRect Rect{};
        };

        Limen::LStaticArray<Button, 4> m_Buttons{
            {SceneToolbarAction::New, "New", 51.0f},
            {SceneToolbarAction::Open, "Open", 55.0f},
            {SceneToolbarAction::Save, "Save", 53.0f},
            {SceneToolbarAction::SaveAs, "Save As", 70.0f}
        };
    };
}
