#version 410 core

/**
 * CPU 已经使用每个 Quad 的 Model 矩阵，
 * 将局部坐标转换为了世界空间坐标。
 *
 * 对应 QuadVertex::Position。
 */
layout(location = 0) in vec3 a_Position;

/**
 * 当前顶点的 RGBA 颜色。
 *
 * 对应 QuadVertex::Color。
 * 光栅化阶段会在三角形内部对颜色进行插值。
 */
layout(location = 1) in vec4 a_Color;

/**
 * 当前二维场景的 Projection × View 矩阵。
 *
 * 一整个批次共用同一个相机矩阵。
 */
uniform mat4 u_ViewProjection;

/**
 * 传给 Fragment Shader 的顶点颜色。
 */
out vec4 v_Color;

void main()
{
    // 将顶点颜色交给光栅化阶段进行插值。
    v_Color = a_Color;

/*
     * a_Position 已经是世界空间坐标，
     * 因此不再乘每个 Quad 独立的 u_Transform。
     */
    gl_Position =
    u_ViewProjection *
    vec4(a_Position, 1.0);
}