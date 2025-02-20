#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "rc_common.h"

layout(binding = 0) readonly uniform Transform
{
    TransformData trans;
};

layout(binding = 1) readonly buffer Vertices
{
    Vertex vertices [];
};

void main()
{
    uint vi = gl_VertexIndex;
    vec3 pos = vec3(int(vertices[vi].nx), int(vertices[vi].ny), int(vertices[vi].nz));

    gl_Position = trans.proj * trans.view * vec4(pos, 1.0);
}