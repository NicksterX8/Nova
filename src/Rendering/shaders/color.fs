#version 330 core

out vec4 FragColor;
in vec4 Color;
in vec2 TexCoord;

uniform sampler2D tex;
uniform vec2 texSize;

void main() {
    if (TexCoord.x != 0.0 && TexCoord.y != 0.0) {
        FragColor = texture(tex, TexCoord / texSize);
    } else {
        FragColor = Color;
    }
}