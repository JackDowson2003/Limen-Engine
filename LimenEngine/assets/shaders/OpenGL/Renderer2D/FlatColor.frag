#version 410 core

/**
 * 当前片元最终写入颜色缓冲区的RGBA颜色。
 */
layout(location = 0) out vec4 color;

// MaterialData 对应绑定到该 uniform block 的材质 UBO。
// std140 中 vec4 的大小和基础对齐均为16字节。
layout(std140) uniform MaterialData
{
    vec4 u_Color;
};

void main()
{
    color = u_Color;
}
