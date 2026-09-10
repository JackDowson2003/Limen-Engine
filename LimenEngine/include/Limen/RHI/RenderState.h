//
// Created by chenlong on 2026/8/23.
//
#pragma once
#include <cstdint>

/**
 * 为什么需要这些函数(operator | & &&)
 * 是强类型枚举，C++ 默认不允许这样写：ClearFlags::Color | ClearFlags::Depth
 * 所以添加函数
 */
namespace Limen
{
    /**
     * @brief 制定一次Clear操作需要清理哪些FBO附件
     */
    enum class ClearFlags : uint8_t
    {
        None = 0,
        Color = 1u << 0,
        Depth = 1u << 1,
        Stencil = 1u << 2,
    };

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
        // 表示不进行混合，新的颜色直接覆盖旧颜色：
        Opaque = 0,
        // a * c1 + (1 - a ) c2 按照alpha 混合
        AlphaBlend,
        // 表示把新的颜色直接加到已有颜色上
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

    /**
     * @brief 将两个清理标记起来
     *
     * 例如:
     * ClearFlags::Color | ClearFlags::Depth
     * 表示同时清理颜色和深度
     */
    constexpr ClearFlags operator|(ClearFlags left, ClearFlags right) noexcept
    {
        return static_cast<ClearFlags>
        (
            static_cast<uint8_t>(left) |
            static_cast<uint8_t>(right)
        );
    }

    /**
     * @brief 把新的清理标记追加到已有标记中。
     *
     * 例如：
     * flags |= ClearFlags::Depth;
     */
    constexpr ClearFlags& operator|=(ClearFlags& left, const ClearFlags right) noexcept
    {
        left = left | right;
        return left;
    }

    /**
     * @brief 判断一组清理标记中是否包含指定标记。
     *
     * 例如：
     * HasClearFlag(flags, ClearFlags::Color)
     * 1101 & 0001 = 0001 ### It's wrong in this environment ###
     * So we can just write (xxx) != 0
     */
    inline bool HasClearFlag(ClearFlags flags, ClearFlags right) noexcept
    {
        return( static_cast<uint8_t>(flags) & static_cast<uint8_t>(right)) != 0;
    }

}
