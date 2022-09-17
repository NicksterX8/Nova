#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec3 textColor;
//uniform float minD;
uniform float thickness;
uniform float soft;

void main() {   
    float a = texture(text, TexCoords).r;
    a = smoothstep(1.0 - thickness - soft, 1.0 - thickness + soft, a);
    FragColor = vec4(textColor, a);
}