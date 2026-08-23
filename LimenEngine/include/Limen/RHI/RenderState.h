//
// Created by chenlong on 2026/8/23.
//
#pragma once

namespace Limen
{
    /**
     * @brief GPU 将输入顶点组装为图元的方式。
     *
     * OpenGL 在绘制命令中使用该状态；Direct3D 12 会将其映射到
     * Input Assembler 的 Primitive Topology。
     */
    enum class PrimitiveTopology
    {
        TriangleList = 0,
        LineList,
        PointList
    };

    /** @brief 指定光栅化前需要剔除的三角形面。 */
    enum class CullMode
    {
        None = 0,
        Front,
        Back
    };
    /**
     * @brief 指定哪一种顶点绕序代表三角形正面。
     */
    enum class FrontFace
    {
        CounterClockwise = 0,
        Clockwise
    };

    /**
     * @brief 管线写入颜色附件时使用的混合模式。
     */
    enum class BlendMode
    {
        Opaque = 0,
        AlphaBlend,
        Additive
    };

    /** @brief 深度测试比较新片元与深度缓冲值的方式。 */
    enum class CompareOperation
    {
        Never = 0,
        Less,
        LessEqual,
        Greater,
        GreaterEqual,
        Equal,
        NotEqual,
        Always
    };
}
