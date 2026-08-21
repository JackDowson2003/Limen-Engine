#version 410 core

/**
 * 顶点在模型局部空间中的位置。
 *
 * location = 0对应VBO布局中的Float3 Position。
 */
layout(location = 0) in vec3 a_Position;

/**
 * 顶点的二维纹理坐标。
 *
 * location = 1对应VBO布局中的Float2 TexCoord。
 */
layout(location = 1) in vec2 a_TexCoord;

/**
 * 当前相机的Projection × View矩阵。
 */
uniform mat4 u_ViewProjection;

/**
 * 当前物体从局部空间到世界空间的Model矩阵。
 */
uniform mat4 u_Transform;

/**
 * 传递给Fragment Shader的纹理坐标。
 * 光栅化阶段会对它进行透视正确插值。
 */
out vec2 v_TexCoord;

void main()
{
    v_TexCoord = a_TexCoord;

    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}