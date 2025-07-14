#version 330 core

out vec4 FragColor;
in vec4 Color;
in vec2 TexCoord;

uniform sampler2D tex;
uniform vec2 texSize;

void main() {
    if (TexCoord.x <= texSize.x && TexCoord.y <= texSize.y) {
        FragColor = texture(tex, TexCoord / texSize);
    } else {
        FragColor = Color;
    }
}