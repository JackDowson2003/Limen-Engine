#version 410 core

/**
 * 当前片元最终写入颜色缓冲区的RGBA颜色。
 */
layout(location = 0) out vec4 color;

/**
 * 当前物体的纯色。
 *
 * RGB三个分量通常位于[0, 1]范围。
 * 由CPU在绘制前上传。
 */
//uniform vec3 u_Color;

layout(std140) uniform MaterialData
{
    vec3 u_Color;
};

void main()
{
    color = vec4(u_Color, 1.0);
}
