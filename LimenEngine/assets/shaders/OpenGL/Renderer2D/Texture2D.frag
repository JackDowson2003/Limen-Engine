#version 410 core

/**
 * 当前片元最终写入颜色缓冲区的RGBA颜色。
 */
layout(location = 0) out vec4 color;

/**
 * 经过光栅化插值后的二维纹理坐标。
 */
in vec2 v_TexCoord;

/**
 * 二维纹理采样器。
 *
 * 当前由C++设置为纹理槽0。
 */
uniform sampler2D u_Texture;

void main()
{
    color = texture(u_Texture, v_TexCoord);
}