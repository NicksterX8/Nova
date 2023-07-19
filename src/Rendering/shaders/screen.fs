#version 330 core

out vec4 FragColor;
in vec2 TexCoord;
in vec2 Velocity;

uniform sampler2D tex;
uniform int numSamples;

void main() {
    vec2 texCoord = TexCoord;
    vec4 color = texture(tex, texCoord);
    vec2 v = vec2(Velocity.x / numSamples, Velocity.y / numSamples);
    texCoord += v;
    for (int i = 1; i < numSamples; ++i, texCoord += v) {
        // Sample the color buffer along the velocity vector.
        vec4 currentColor = texture(tex, texCoord);
        // Add the current color to our color sum.
        color += currentColor;
    }

    // Average all of the samples to get the final blur color.
    FragColor = color / numSamples; 

    if (FragColor.a < 0.1) {
        discard;
    }
}