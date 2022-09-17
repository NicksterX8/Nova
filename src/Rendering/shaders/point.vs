#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aPointSize;

out vec4 Color;

uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(aPos, 1.0);
    gl_PointSize = aPointSize;
    Color = aColor;
}