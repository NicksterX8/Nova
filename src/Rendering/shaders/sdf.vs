#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec2 aScale;

out vec2 TexCoord;
out vec4 TextColor;

uniform mat4 transform;

void main() {
    mat4 t = transform;
    vec4 scale = vec4(aScale, 1.0, 1.0);
    mat4 trans;
    trans[0] = transform[0] * scale;
    trans[1] = transform[1] * scale;
    trans[2] = transform[2] * scale;
    trans[3] = transform[3];

    gl_Position = trans * vec4(aPos, 0.0, 1.0);
    //aPos *= aScale;
    TexCoord = aTexCoord;
    TextColor = aColor;
}