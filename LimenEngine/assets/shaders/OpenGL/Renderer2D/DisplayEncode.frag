#version 410 core

in vec2 v_TexCoord;
layout(location = 0) out vec4 o_Color;

uniform sampler2D u_SceneColor;

float LinearChannelToSRGB(float linearValue)
{
    linearValue = clamp(linearValue, 0.0, 1.0);

    if (linearValue <= 0.0031308)
        return linearValue * 12.92;

    return 1.055 * pow(linearValue, 1.0 / 2.4) - 0.055;
}

void main()
{
    vec4 sceneColor = texture(u_SceneColor, v_TexCoord);

    // 只编码显示用 RGB；Alpha 不是颜色亮度，不做 sRGB 转换。
    o_Color = vec4(
        LinearChannelToSRGB(sceneColor.r),
        LinearChannelToSRGB(sceneColor.g),
        LinearChannelToSRGB(sceneColor.b),
        sceneColor.a
    );
}