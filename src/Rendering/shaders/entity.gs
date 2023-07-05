#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec3 Pos;
    vec4 TexCoords;
    vec2 Size;
    float Rotation;
    float Texture;
} gs_in[];

out vec3 TexCoord;

uniform mat4 transform;

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
    gl_Position = transform * rotated;
    EmitVertex();
}

void main() {
    vec4  TexCoords = gs_in[0].TexCoords;
    vec3  pos       = gs_in[0].Pos;
    vec2  size      = gs_in[0].Size;
    float rotation  = gs_in[0].Rotation;

    float angle = rotation * PI180;
    c = cos(angle);
    s = sin(angle);

    TexCoord.z = gs_in[0].Texture;
    TexCoord.xy = vec2(TexCoords.x, TexCoords.y);
    make_vertex(pos, vec2(-size.x, -size.y));

    TexCoord.xy = vec2(TexCoords.x, TexCoords.w);
    make_vertex(pos, vec2(-size.x,  size.y));

    TexCoord.xy = vec2(TexCoords.z, TexCoords.y);
    make_vertex(pos, vec2( size.x,  -size.y));

    TexCoord.xy = vec2(TexCoords.z, TexCoords.w);
    make_vertex(pos, vec2( size.x,  size.y));

    EndPrimitive();
}