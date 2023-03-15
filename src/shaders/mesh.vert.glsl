#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh.h"

layout(push_constant) uniform block 
{
    Constants constants;
};


layout(binding = 0) readonly buffer Vertices
{
    Vertex vertices[];
};

layout(location = 0) out vec4 color;

void main()
{
    vec3 pos = vec3(vertices[gl_VertexIndex].vx, vertices[gl_VertexIndex].vy, vertices[gl_VertexIndex].vz);
    vec3 norm = vec3(int(vertices[gl_VertexIndex].nx), int(vertices[gl_VertexIndex].ny), int(vertices[gl_VertexIndex].nz)) / 127.0 - 1;
    vec2 uv = vec2(vertices[gl_VertexIndex].tu, vertices[gl_VertexIndex].tv);

    vec3 result = vec3(pos * vec3(constants.scale, 1.0) + vec3(constants.offset, 0.0) * vec3(2, 2, 0.5) + vec3(-1, -1, 0.5));

    gl_Position = vec4(result, 1.0);

    color = vec4(norm * 0.5 + vec3(0.5), 1.0);
}
