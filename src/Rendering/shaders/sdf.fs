#version 330 core

in vec2 TexCoord;
in vec4 TextColor;

out vec4 FragColor;

//uniform float thickness;
//uniform float soft;
uniform sampler2D text;

void main() {   
    float d = 1.0 - texture(text, TexCoord).r;

    float soft = 0.1;
    float thickness = 0.45;
    float weight = thickness + soft;

    float a;
    if (d < thickness) {
        a = 1.0;
    }
    else if (d > weight) {
        discard;
    } else {
        a = smoothstep(1.0, 0.0, (d - thickness) / soft);

    }

    FragColor = vec4(TextColor.xyz, TextColor.w * a);
}