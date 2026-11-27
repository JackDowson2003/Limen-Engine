//
// Created by chenlong on 2026/10/16.
//

#pragma once
#include <cstdint>

#include "Limen/Scene/Scene.h"

namespace LimenEditor
{
    inline constexpr float kEditorWideToolbarWidth = 900.0f;
    inline constexpr float kDefaultEditorRightSplitRatio = 0.42f;

    // 左上角原点的逻辑窗口矩形；只保存数值，不拥有窗口或 GPU 资源。
    struct UIRect
    {
        float X = 0.0f;      // 左边界
        float Y = 0.0f;      // 上边界
        float Width = 0.0f;  // 逻辑宽度
        float Height = 0.0f; // 逻辑高度

        // x、y 是逻辑窗口坐标；右、下边界不包含在矩形内。
        [[nodiscard]] bool Contains(float x, float y) const
        {
            return Width > 0.0f && Height > 0.0f
                && x >= X && x < X + Width
                && y >= Y && y < Y + Height;
        }
    };

    // 逻辑矩形换算成帧缓冲像素尺寸；至少 1×1，输入无效时同样返回 1×1。
    struct ViewportPixelSize
    {
        uint32_t Width = 1;
        uint32_t Height = 1;
    };

    [[nodiscard]] ViewportPixelSize CalculateViewportPixelSize(
        float logicalWindowWidth,
        float logicalWindowHeight,
        const UIRect& logicalRect,
        uint32_t framebufferWidth,
        uint32_t framebufferHeight
    );

    // 一次布局计算的结果；尚未包含绘制命令或面板对象。
    struct EditorUILayoutResult
    {
        UIRect Top{};              // 全局顶部工具栏
        UIRect ViewportToolbar{};  // 视口操作区，位于实际渲染区域之上
        UIRect Center{};           // 左侧视口渲染区域
        UIRect Outliner{};         // 右侧上方的场景对象树
        UIRect DetailsSplitter{}; // 对象树与属性面板之间的横向分隔条
        UIRect Inspector{};        // 右侧下方的属性面板
        UIRect ContentSplitter{};  // 资源抽屉上沿的横向分隔条
        UIRect Content{};          // 视口下方的资源抽屉
        UIRect Status{};           // 底部状态和抽屉入口
    };

    // 尺寸与输出使用左上角原点的逻辑坐标。抽屉与 Outliner 的比例均基于各自扣除分隔条后的高度。
    // 只计算区域，不读取窗口状态或提交绘制命令。
    [[nodiscard]] EditorUILayoutResult CalculateEditorUILayout(
        float logicalWidth,
        float logicalHeight,
        float contentHeightRatio,
        bool showContent,
        bool viewportExpanded,
        float rightSplitRatio = kDefaultEditorRightSplitRatio
    );

    // 根据首行 Y、行高和鼠标 Y 求列表行号；不在任何行内返回 -1。
    [[nodiscard]] int FindListRowAt(
        float firstRowY,
        float rowHeight,
        float mouseY,
        uint32_t rowCount
    ) noexcept;

    // 在 Outliner 行顺序中，从 anchor 到 target 生成连续选择范围。
    // anchor 不存在时只返回 target；target 不存在时返回空。
    [[nodiscard]]
    Limen::LArray<Limen::SceneObjectID> BuildSelectionRange(
        const Limen::LArray<Limen::SceneObjectID>& orderedRows,
        Limen::SceneObjectID anchor,
        Limen::SceneObjectID target
    );

}
