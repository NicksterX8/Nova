#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aTexCoord;

out VS_OUT {
    vec2 Pos;
    vec4 TexCoord;
} vs_out;

void main() {
    vs_out.Pos = aPos;
    vs_out.TexCoord = aTexCoord;
}