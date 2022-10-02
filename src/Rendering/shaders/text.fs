#version 330 core
in uint TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec3 textColor;

void main() {    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texelFetch(text, TexCoords).r);
    FragColor = vec4(textColor, 1.0) * sampled;
}