#version 410 core

/**
 * 光栅化阶段插值得到的片元颜色。
 *
 * 如果一个 Quad 的四个顶点颜色相同，
 * 整个 Quad 就会显示为统一颜色。
 */
in vec4 v_Color;

/**
 * 当前片元最终写入颜色附件的 RGBA 颜色。
 */
layout(location = 0) out vec4 color;

void main()
{
    color = v_Color;
}