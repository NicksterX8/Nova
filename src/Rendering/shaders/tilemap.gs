#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec2 Pos;
    vec3 TexCoord;
} gs_in[];

out vec3 TexCoord;

uniform mat4 transform;
uniform vec2 texArraySize;

void make_vertex(vec2 pos, vec2 offset) {
    gl_Position = transform * vec4(pos.xy + offset, 0.0, 1.0);
    EmitVertex();
}

void main() {
    vec3  texCoord = gs_in[0].TexCoord;
    vec2  pos      = gs_in[0].Pos;

    vec2 texMin = vec2(0.01, 0.01) / texArraySize;
    vec2 texMax = vec2(texCoord.x - 0.01, texCoord.y - 0.01) / texArraySize;

    TexCoord.z = texCoord.z;
    TexCoord.xy = vec2(texMin.x, texMin.y);
    make_vertex(pos, vec2(0.0, 0.0));

    TexCoord.xy = vec2(texMin.x, texMax.y);
    make_vertex(pos, vec2(0.0,  1.0));

    TexCoord.xy = vec2(texMax.x, texMin.y);
    make_vertex(pos, vec2(1.0,  0.0));

    TexCoord.xy = vec2(texMax.x, texMax.y);
    make_vertex(pos, vec2(1.0,  1.0));

    EndPrimitive();
}