#version 410 core

/**
 * 模型局部空间中的顶点位置。
 */
layout(location = 0) in vec3 a_Position;

/**
 * 模型局部空间中的顶点法线。
 */
layout(location = 1) in vec3 a_Normal;

/**
 * 顶点的二维纹理坐标。
 */
layout(location = 2) in vec2 a_TexCoord;

/**
 * 当前相机的Projection × View矩阵。
 */
uniform mat4 u_ViewProjection;

/**
 * 当前物体从模型局部空间到世界空间的Model矩阵。
 */
uniform mat4 u_Transform;

/**
 * 传递给Fragment Shader的世界空间法线。
 */
out vec3 v_WorldNormal;

/**
 * 传递给Fragment Shader的着色点世界坐标。
 */
out vec3 v_WorldPosition;

/**
 * 传递给Fragment Shader的纹理坐标。
 */
out vec2 v_TexCoord;

void main()
{
     /**
     * 法线矩阵：
     *
     * transpose(inverse(mat3(Model)))
     *
     * 在物体存在非均匀缩放时，保证变换后的法线
     * 仍然垂直于对应表面。
     */
    mat3 normalMatrix = transpose(inverse(mat3(u_Transform)));

    v_WorldNormal = normalize(normalMatrix * a_Normal);

    v_TexCoord = a_TexCoord;

    vec4 worldPosition = u_Transform * vec4(a_Position, 1.0);

    v_WorldPosition = worldPosition.xyz;

    gl_Position = u_ViewProjection * worldPosition;
}