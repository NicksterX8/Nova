#version 330 core

in vec2 TexCoord;
in vec4 TextColor;

out vec4 FragColor;

uniform sampler2D text;

uniform float thickness;
uniform float soft;
uniform float outlineDistance;
uniform float smoothing;
uniform vec4 outlineColor;

uniform vec2 shadowOffset; // Between 0 and spread / textureSize
uniform float shadowSmoothing; // Between 0 and 0.5
uniform vec4 shadowColor;

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
    float d = texture(text, TexCoord).r;
    float outlineFactor = smoothstep(0.5 - smoothing, 0.5 + smoothing, d);
    vec4 color = mix(outlineColor, TextColor, outlineFactor);
    float alpha = smoothstep(outlineDistance - smoothing, outlineDistance + smoothing, d);

    /*
    float shadowDistance = texture(text, TexCoord - shadowOffset).r;
    float shadowAlpha = smoothstep(0.5 - shadowSmoothing, 0.5 + shadowSmoothing, shadowDistance);
    vec4 shadow = vec4(shadowColor.rgb, shadowColor.a * shadowAlpha);
    vec4 textColor = vec4(color.rgb, color.a * alpha);
    FragColor = mix(shadow, textColor, textColor.a);
    */
    FragColor = vec4(color.rgb, color.a * alpha);
    
}