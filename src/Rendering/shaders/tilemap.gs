#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec2 Pos;
    vec4 TexCoord;
} gs_in[];

out vec2 TexCoord;

uniform mat4 transform;
uniform vec2 texSize;
uniform float height;

void make_vertex(vec2 pos, vec2 offset) {
    gl_Position = transform * vec4(pos.xy + offset, height, 1.0);
    EmitVertex();
}

void main() {
    vec2  pos      = gs_in[0].Pos;

    vec2 texMin = (gs_in[0].TexCoord.xy + vec2(0.01, 0.01)) / texSize;
    vec2 texMax = (gs_in[0].TexCoord.zw - vec2(0.01, 0.01)) / texSize;

    TexCoord = vec2(texMin.x, texMin.y);
    make_vertex(pos, vec2(0.0, 0.0));

    TexCoord = vec2(texMin.x, texMax.y);
    make_vertex(pos, vec2(0.0,  1.0));

    TexCoord = vec2(texMax.x, texMin.y);
    make_vertex(pos, vec2(1.0,  0.0));

    TexCoord = vec2(texMax.x, texMax.y);
    make_vertex(pos, vec2(1.0,  1.0));

    EndPrimitive();
}