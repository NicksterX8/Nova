#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

#define minD 0.1
#define maxD 3.0

uniform sampler2D text;
uniform vec3 textColor;

void main() {   
    float dist = texture(text, TexCoords).r;
    if (dist < minD) {
        FragColor = vec4(0,0,0,0);
    }
    else if (dist > maxD) {
        FragColor = vec4(0,0,0,0);
    } else {
        FragColor = vec4(textColor, 1.0); // maybe 1.0 shouldn't be a constant there, text color could be vec4
    }
}