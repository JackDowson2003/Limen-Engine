#version 410 core

/**
 * CPU 已经使用 Model 矩阵计算好的世界空间位置。
 */
layout(location = 0) in vec3 a_Position;

/**
 * 当前顶点的 RGBA 颜色。
 *
 * 纯色 Quad 直接输出该颜色；
 * 纹理 Quad 使用该颜色对纹理进行 Tint。
 */
layout(location = 1) in vec4 a_Color;

/**
 * 当前顶点的二维纹理坐标。
 */
layout(location = 2) in vec2 a_TexCoord;

/**
 * 当前顶点使用的纹理槽编号。
 *
 * 0～15 表示有效纹理槽；
 * 16 表示不使用纹理。
 */
layout(location = 3) in uint a_TextureIndex;

/**
 * 当前二维场景共用的 Projection × View 矩阵。
 */
uniform mat4 u_ViewProjection;

/**
 * 传递给 Fragment Shader 的颜色和纹理坐标。
 */
out vec4 v_Color;
out vec2 v_TexCoord;

/**
 * 整数纹理索引不能进行插值。
 *
 * flat 保证整个三角形使用同一个纹理槽编号。
 */
flat out uint v_TextureIndex;

void main()
{
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    v_TextureIndex = a_TextureIndex;

    /*
     * a_Position 已经是世界空间坐标，
     * 因此这里只需要乘 ViewProjection。
     */
    gl_Position =
    u_ViewProjection *
    vec4(a_Position, 1.0);
}