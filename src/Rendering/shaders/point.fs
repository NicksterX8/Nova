#version 330 core

out vec4 FragColor;
in vec4 Color;

void main() {
    FragColor = Color;
    if (FragColor.a < 0.1) {
        discard;
    }
}