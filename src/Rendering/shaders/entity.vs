#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aTexCoord;
layout (location = 2) in vec3 aRotation;

out vec3 TexCoord;

uniform mat4 transform;
uniform vec2 texArraySize;

#define PI180 0.01745329252

void main() {
    float rotation = aRotation.x;
    vec2 rotationCenter = aRotation.yz;
    float angle = rotation * PI180;
    float c = cos(angle);
    float s = sin(angle);
    /*
    mat4 rotationMatrix = mat4(
        cos(angle), sin(angle), 0, 0,
        -sin(angle), cos(angle), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
    */
    // translate to center of entity
    vec4 relPos = vec4(aPos.xy - rotationCenter, aPos.z, 1.0);
    // rotate
    vec4 rotatedPos = vec4(relPos.x * c + relPos.y * -s, relPos.x * s + relPos.y * c, aPos.z, 1.0);
    // translate back
    rotatedPos.xy += rotationCenter;
    
    gl_Position = transform * rotatedPos;
    TexCoord.xy = aTexCoord.xy / texArraySize;
    TexCoord.z = aTexCoord.z;
}