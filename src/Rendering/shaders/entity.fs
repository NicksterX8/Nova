#version 330 core

layout (location = 0) out vec4 FragColor;

in vec2 TexCoord;
in vec4 Color;

uniform sampler2D texAtlas;

void main() {
    vec4 texColor = texture(texAtlas, TexCoord);
    if (texColor.a < 0.1) {
        discard;
    }
    FragColor = vec4(Color.xyz * texColor.xyz, Color.a);
}