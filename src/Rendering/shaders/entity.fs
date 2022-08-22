#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2DArray texArray;

uniform int texLayer;

void main() {
    FragColor = texture(texArray, vec3(TexCoord.x, TexCoord.y, texLayer));
    if (FragColor.a < 0.1) {
        discard;
    }
}