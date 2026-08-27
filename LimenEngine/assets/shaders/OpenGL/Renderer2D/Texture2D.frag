#version 410 core

/**
 * 从 Vertex Shader 经过光栅化阶段传入的数据。
 */
in vec4 v_Color;
in vec2 v_TexCoord;

/**
 * 纹理槽编号不能进行插值。
 */
flat in uint v_TextureIndex;

/**
 * 当前片元最终写入颜色附件的 RGBA 颜色。
 */
layout(location = 0) out vec4 color;

/**
 * 当前批次最多同时绑定16张二维纹理。
 *
 * 数组下标必须和 CPU 端 TextureSlots 的下标一致。
 * 表示 Fragment Shader 最多能够通过这16个采样器访问16个纹理单元。
 */
uniform sampler2D u_Textures[16];

void main()
{
    /*
     * CPU 使用16表示纯色 Quad。
     *
     * 有效纹理槽范围为0～15，因此16不会和真实纹理冲突。
     */
    if (v_TextureIndex == 16u)
    {
        color = v_Color;
        return;
    }

/*
     * 默认使用醒目的紫红色表示无效纹理索引。
     * 正常情况下该值会被下面的有效纹理采样覆盖。
     */
    vec4 textureColor = vec4(1.0, 0.0, 1.0, 1.0);

    /*
     * GLSL 4.10 对 sampler 数组的动态索引有限制。
     *
     * 使用 switch 可以保证每个 sampler 下标都是编译期常量，
     * 从而兼容 macOS OpenGL 4.1。
     */
    switch (v_TextureIndex)
    {
        case 0u:  textureColor = texture(u_Textures[0],  v_TexCoord); break;
        case 1u:  textureColor = texture(u_Textures[1],  v_TexCoord); break;
        case 2u:  textureColor = texture(u_Textures[2],  v_TexCoord); break;
        case 3u:  textureColor = texture(u_Textures[3],  v_TexCoord); break;
        case 4u:  textureColor = texture(u_Textures[4],  v_TexCoord); break;
        case 5u:  textureColor = texture(u_Textures[5],  v_TexCoord); break;
        case 6u:  textureColor = texture(u_Textures[6],  v_TexCoord); break;
        case 7u:  textureColor = texture(u_Textures[7],  v_TexCoord); break;
        case 8u:  textureColor = texture(u_Textures[8],  v_TexCoord); break;
        case 9u:  textureColor = texture(u_Textures[9],  v_TexCoord); break;
        case 10u: textureColor = texture(u_Textures[10], v_TexCoord); break;
        case 11u: textureColor = texture(u_Textures[11], v_TexCoord); break;
        case 12u: textureColor = texture(u_Textures[12], v_TexCoord); break;
        case 13u: textureColor = texture(u_Textures[13], v_TexCoord); break;
        case 14u: textureColor = texture(u_Textures[14], v_TexCoord); break;
        case 15u: textureColor = texture(u_Textures[15], v_TexCoord); break;
        default: break;
    }

    /*
     * 顶点颜色作为纹理 Tint。
     *
     * 传入白色 vec4(1.0) 时保持纹理原色；
     * 修改颜色可以实现染色；
     * 修改 alpha 可以控制整体透明度。
     */
    color = textureColor * v_Color;
}