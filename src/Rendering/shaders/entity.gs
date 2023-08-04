#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec3  Pos;
    vec4  TexCoords;
    vec2  Size;
    float Rotation;
    vec2  Velocity;
} gs_in[];

out vec2 TexCoord;
out vec2 Velocity;

uniform mat4 transform;

mat4 trans;
vec2 velocity;

float c;
float s;

#define PI180 0.01745329252

vec2 rotate(vec2 point) {
    return vec2(point.x * c + point.y * -s, point.x * s + point.y * c);
}

void make_vertex(vec3 pos, vec2 offset) {
    // rotate
    vec4 rotated = vec4(rotate(offset), pos.z, 1.0);
    // translate back
    rotated.xy += pos.xy;
    vec4 newNormalizedPos = trans * rotated;

    rotated.xy -= velocity;

    vec4 oldNormalizedPos = trans * rotated;
    vec2 normalizedVelocity = newNormalizedPos.xy - oldNormalizedPos.xy;

    gl_Position = newNormalizedPos;
    Velocity = oldNormalizedPos.xy;
    //if (normalizedVelocity.x < 1.0 && normalizedVelocity.x > 0.0) {
        EmitVertex();
    //}
}

void main() {
    vec4  TexCoords = gs_in[0].TexCoords;
    vec3  pos       = gs_in[0].Pos;
    vec2  size      = gs_in[0].Size;
    float rotation  = gs_in[0].Rotation;
          velocity  = gs_in[0].Velocity;

    trans = transform;
    /*
    vec3 translation = trans[3].xyz;
    trans[3] = vec4(0, 0, 0, trans[3].w);
    vec4 transformedVelocity = vec4(velocity, 0.0, 1.0) * trans;
    // TODO: this probably should be affected by rotation
    Velocity = transformedVelocity.xy;
    trans[3] = vec4(translation, trans[3].w);
    */

    float angle = rotation * PI180;
    c = cos(angle);
    s = sin(angle);

    TexCoord = vec2(TexCoords.x, TexCoords.y);
    make_vertex(pos, vec2(-size.x, -size.y));

    TexCoord = vec2(TexCoords.x, TexCoords.w);
    make_vertex(pos, vec2(-size.x,  size.y));

    TexCoord = vec2(TexCoords.z, TexCoords.y);
    make_vertex(pos, vec2( size.x,  -size.y));

    TexCoord = vec2(TexCoords.z, TexCoords.w);
    make_vertex(pos, vec2( size.x,  size.y));

    EndPrimitive();
}