#version 410 core

in vec2 v_TexCoord;
layout(location = 0) out vec4 o_Color;

uniform sampler2D u_SceneColor;

uniform float u_Exposure;

float LinearChannelToSRGB(float linearValue)
{
    linearValue = max(linearValue, 0.0);

    if (linearValue <= 0.0031308)
        return linearValue * 12.92;

    return 1.055 * pow(linearValue, 1.0 / 2.4) - 0.055;
}

vec3 LinearToSRGB(vec3 linearColor)
{
    return vec3(
    LinearChannelToSRGB(linearColor.r),
    LinearChannelToSRGB(linearColor.g),
    LinearChannelToSRGB(linearColor.b)
    );
}

vec3 ToneMapReinhard(vec3 hdrColor)
{
    vec3 nonNegativeColor = max(hdrColor,vec3(0.0));
    return nonNegativeColor / (vec3(1.0) + nonNegativeColor);
}

void main()
{
    vec4 sceneColor = texture(u_SceneColor, v_TexCoord);
    vec3 exposedColor = sceneColor.rgb * u_Exposure;
    vec3 displayLinearColor = ToneMapReinhard(exposedColor);

    // Scene 视口的最终图像不透明；Alpha 不再透出 ImGui 底色。
    o_Color = vec4(LinearToSRGB(displayLinearColor), 1.0);
}