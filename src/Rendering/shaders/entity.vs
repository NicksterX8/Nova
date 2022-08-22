#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 transform;
uniform float angle;

void main() {
    mat4 rotationMatrix = mat4(
        cos(angle), sin(angle), 0, 0,
        -sin(angle), cos(angle), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
    gl_Position = rotationMatrix * transform * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}