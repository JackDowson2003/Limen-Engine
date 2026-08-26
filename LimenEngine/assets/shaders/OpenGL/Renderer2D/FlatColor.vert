#version 410 core

/**
 * 顶点在模型局部空间中的位置。
 *
 * location = 0对应VBO布局中的Position。
 */
layout(location = 0) in vec3 a_Position;

/**
 * 当前相机的Projection × View矩阵。
 */
uniform mat4 u_ViewProjection;

/**
 * 当前物体从局部空间到世界空间的Model矩阵。
 */
uniform mat4 u_Transform;

void main()
{
    gl_Position =
    u_ViewProjection *
    u_Transform *
    vec4(a_Position, 1.0);
}