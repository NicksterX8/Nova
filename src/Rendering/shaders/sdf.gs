#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec2 Pos;
    vec2 TexCoord;
    vec2 Size;
    vec2 Scale;
    vec4 Color;
} gs_in[];

out vec2 TexCoord;
out vec4 TextColor;

uniform mat4 transform;
uniform vec2 texSize;

mat4 trans;

void make_vertex(vec2 pos, vec2 offset) {
    gl_Position = trans * vec4(pos + offset, 0, 1.0);
    EmitVertex();
}

void main() {
    vec2 pos      = gs_in[0].Pos;
    vec2 size     = gs_in[0].Size;
    vec2 texCoord = gs_in[0].TexCoord;

    vec2 texMin = (texCoord + vec2(0.01, 0.01)) / texSize;
    vec2 texMax = (texCoord + size - vec2(0.01, 0.01)) / texSize;

    vec4 scale = vec4(gs_in[0].Scale, 1.0, 1.0);
    trans[0] = transform[0] * scale;
    trans[1] = transform[1] * scale;
    trans[2] = transform[2] * scale;
    trans[3] = transform[3];

    TextColor = gs_in[0].Color;

    TexCoord = vec2(texMin.x, texMin.y);
    make_vertex(pos, vec2(0.0, size.y));

    TexCoord = vec2(texMin.x, texMax.y);
    make_vertex(pos, vec2(0.0, 0.0));

    TexCoord = vec2(texMax.x, texMin.y);
    make_vertex(pos, vec2(size.x, size.y));

    TexCoord = vec2(texMax.x, texMax.y);
    make_vertex(pos, vec2(size.x, 0.0));

    EndPrimitive();
}