#version 410 core

out vec2 v_TexCoord;

const vec2 kClipPositions[3] = vec2[3](
vec2(-1.0, -1.0),
vec2( 3.0, -1.0),
vec2(-1.0,  3.0)
);

void main()
{
    vec2 clipPosition = kClipPositions[gl_VertexID];

    v_TexCoord = clipPosition * 0.5 + 0.5;
    gl_Position = vec4(clipPosition, 0.0, 1.0);
}