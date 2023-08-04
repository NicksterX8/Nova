/*
 * Motion blur implementation based off of example in NVIDIA's GPU Gems 3 - Chapter 27
 * https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-27-motion-blur-post-processing-effect
 */

#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec2 Velocity;

uniform sampler2D tex;
uniform sampler2D velocityBuffer;
int numSamples;

void main() {
    vec2 texCoord = TexCoord;
    vec4 color = texture(tex, texCoord);
    /*
    numSamples = 30;
    vec2 oldPos = (texture(velocityBuffer, texCoord).xy);
    vec2 pixelVelocity;
    if (abs(oldPos.x) + abs(oldPos.y) > 0.0) {
        pixelVelocity = gl_FragCoord.xy - oldPos;
        pixelVelocity.x = 0.5;
    } else {
        pixelVelocity = vec2(0.0, 0.0);
    }
    //pixelVelocity = gl_FragCoord.xy - oldPos;
    
    vec2 v = pixelVelocity / numSamples;
    v = v;
    for (int i = 1; i < numSamples; ++i, texCoord += v) {
        // Sample the color buffer along the velocity vector.
        vec4 currentColor = texture(tex, texCoord);
        // Add the current color to our color sum.
        color += currentColor;
    }

    // Average all of the samples to get the final blur color.
    FragColor = color / numSamples; 
    //FragColor = color;

        //FragColor = vec4(FragColor.r, pixelVelocity.x + pixelVelocity.y, FragColor.b, 1.0);

    if (FragColor.a < 0.1) {
       discard;
    }

    

   // FragColor = vec4(pixelVelocity.x * 10, FragColor.g, FragColor.b, 1.0) * FragColor;

    if (abs(oldPos.x) + abs(oldPos.y) > 0.0) {
        FragColor.a = 0.0;
    }
    */
    FragColor = color;
}