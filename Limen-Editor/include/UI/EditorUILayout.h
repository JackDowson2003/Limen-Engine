//
// Created by chenlong on 2026/10/16.
//

#pragma once

namespace LimenEditor
{
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

    // 一次布局计算的结果；尚未包含绘制命令或面板对象。
    struct EditorUILayoutResult
    {
        UIRect Top{};           // 顶部占位区
        UIRect Left{};          // 左侧栏
        UIRect Center{};        // 中央工作区
        UIRect RightTop{};      // 右上区域
        UIRect RightSplitter{}; // 右侧分隔条
        UIRect RightBottom{};   // 右下区域
    };

    // 输入均为左上角原点的逻辑窗口尺寸；rightTopRatio 由调用方保持在 [0, 1]。
    // 只计算区域，不读取窗口状态或提交绘制命令。
    [[nodiscard]] EditorUILayoutResult CalculateEditorUILayout(
        float logicalWidth,
        float logicalHeight,
        float rightTopRatio
    );
}