#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D texAtlas;

void main() {
    FragColor = texture(texAtlas, TexCoord);
    if (FragColor.a < 0.1) {
        discard;
    }
}