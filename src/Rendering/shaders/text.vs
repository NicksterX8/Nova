#version 330 core
layout (location = 0) in vec2 vertex;
layout (location = 1) in uint texCoord;
out vec2 TexCoords;

uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(vertex, 0.0, 1.0);
    TexCoords = texCoord;
}