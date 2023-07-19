#version 330 core

in vec2 TexCoord;
in vec4 TextColor;

out vec4 FragColor;

uniform sampler2D text;

void main() {
    FragColor = TextColor * texture(text, TexCoord).r;
}