#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec2 aSize;
layout (location = 3) in vec2 aScale;
layout (location = 4) in vec4 aColor;

out VS_OUT {
    vec2 Pos;
    vec2 TexCoord;
    vec2 Size;
    vec2 Scale;
    vec4 Color;
} vs_out;

uniform mat4 transform;

void main() {
    vs_out.Pos = aPos;
    vs_out.TexCoord = aTexCoord;
    vs_out.Size = aSize;
    vs_out.Scale = aScale;
    vs_out.Color = aColor;
}