#version 410 core

// shadow pass only care about vertex position, don't care about normal, tex-coords and so on
// 这里是模型的局部坐标
layout(location = 0) in vec3 a_Position;

// 把世界空间位置变换到光源裁剪空间。
uniform mat4 u_LightViewProjection;

// 当前模型从局部空间到世界空间的变换矩阵
uniform mat4 u_Transform;

void main()
{
    gl_Position = u_LightViewProjection * u_Transform * vec4(a_Position,1.0);
}