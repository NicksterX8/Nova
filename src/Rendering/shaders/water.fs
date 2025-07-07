#version 330 core

out vec4 FragColor;
in vec2 fragCoord;
//in vec4 Color;

uniform vec2 iResolution;
uniform float iTime;
uniform float scale;

/*
	Standard Warped Noise
	---------------------

	Loosely based on the original by Shadertoy user "jackdavenport."
	https://www.shadertoy.com/view/Mt2SzR

	In keeping with the concept of the original, this was also coded straight off the top of my head.

*/

// Virtually the same as your original function, just in more compact (and possibly less reliable) form.
float smoothNoise(vec2 p) {
	
	vec2 i = floor(p); p-=i; p *= p*(3.-p-p); 
    
    return dot(mat2(fract(sin(vec4(0, 1, 27, 28) + i.x+i.y*27.) * 1e5))*vec2(1.-p.y,p.y), vec2(1.-p.x, p.x));

}

// Also the same as the original, but with one less layer.
float fractalNoise(vec2 p) {
    
    return smoothNoise(p)*.5333 + smoothNoise(p*2.)*.2667 + smoothNoise(p*4.)*.1333 + smoothNoise(p*8.)*.0667
    ;
    
    // Similar version with fewer layers. The highlighting sample distance would need to be tweaked.
    //return smoothNoise(p)*.57 + smoothNoise(p*2.45)*.28 + smoothNoise(p*6.)*.15;
    
    // Even fewer layers, but the sample distance would need to be tweaked.
    //return smoothNoise(p)*.65 + smoothNoise(p*4.)*.35;
    
}

// Standard noise warping. Call the noise function, then feed a variation of the result
// into itself. Rinse and repeat, etc. Completely made up on the spot, but keeping your 
// original concept in mind, which involved combining noise layers travelling in opposing
// directions.
float warpedNoise(vec2 p) {
    float timeScale = 0.01;
    //vec2 m = vec2(iTime, -iTime)*.5;
    vec2 m = vec2(sin(iTime*timeScale), cos(iTime*timeScale));
    float x = fractalNoise(p + m);
    float y = fractalNoise(p + m.yx + x);
    float z = fractalNoise(p - m - x + y);
    return fractalNoise(p + vec2(x, y) + vec2(y, z) + vec2(z, x) + length(vec3(x, y, z))*0.5);
}


void main() {
    // Screen coordinates. Using division by a scalar, namely "iResolution.y," for aspect correctness.
	vec2 uv = fragCoord.xy;
    
    float noise = fractalNoise(uv);
    
    vec4 color = vec4(0,0,iTime * 0,1);

    if (noise < 0.05) {
        color = vec4(1,1,1,1);
    }

    // Rough gamma correction.
	FragColor = color;
}