#version 330 core

in vec2 TexCoord;
in float FontID;
in vec4 TextColor;

out vec4 FragColor;

uniform sampler2D fontTextures[16];

void main() {
    FragColor = TextColor * texture(fontTextures[int(FontID)], TexCoord).r;
}