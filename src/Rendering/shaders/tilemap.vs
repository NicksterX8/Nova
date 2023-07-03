#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aTexCoord;

out vec3 TexCoord;

uniform mat4 transform;
uniform vec2 texArraySize;

void main() {
    gl_Position = transform * vec4(aPos, 0.0, 1.0); // tilemap rendered at z = 0.0 + transform z translation
    TexCoord = vec3(aTexCoord.xy / texArraySize, aTexCoord.z);
}