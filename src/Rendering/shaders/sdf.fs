#version 330 core

in vec2 TexCoord;
in vec4 TextColor;

out vec4 FragColor;

uniform float thickness;
uniform float soft;
uniform sampler2D text;
uniform float smoothing;

void main() {   
    /*
    float d = 1.0 - texture(text, TexCoord).r;

    float weight = thickness + soft;

    float a;
    if (d < thickness) {
        a = 1.0;
    }
    else if (d > weight) {
        discard;
    } else {
        a = (d - thickness) / soft;
    }

    FragColor = vec4(TextColor.xyz, TextColor.w * a);
    */

    float distance = texture(text, TexCoord).r;
    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
    FragColor = vec4(TextColor.rgb, TextColor.a * alpha);
}