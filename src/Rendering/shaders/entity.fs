#version 330 core

layout (location = 0) out vec4 FragColor;

in vec2 TexCoord;
in vec2 Velocity;

uniform sampler2D texAtlas;

void main() {
    FragColor = texture(texAtlas, TexCoord);
    if (FragColor.a < 0.1) {
        discard;
    }

    //FragVelocity = vec4(gl_FragCoord.x + 0.1, gl_FragCoord.y + 0.1, 0.0, 0.0);
}