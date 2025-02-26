#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_ARB_draw_instanced : require

#extension GL_GOOGLE_include_directive: require
#include "mesh_gpu.h"
#include "rc_common.h"

layout(push_constant) uniform block
{
    VoxDebugConsts info;
};

layout(binding = 0) readonly uniform Transform
{
    TransformData trans;
};

layout(binding = 1) readonly buffer Vertices
{
    Vertex vertices [];
};

layout(binding = 2) readonly buffer VoxDraws
{
    VoxDraw voxDraw [];
};

layout(location = 0) out flat uint out_instId;
layout(location = 1) out vec3 out_color;

void main()
{
    int instId = gl_InstanceIndex;
    VoxDraw draw = voxDraw[instId];
    vec3 vpos = draw.pos;
    vec3 color = draw.col;

    uint vi = gl_VertexIndex;
    vec3 pos = vec3(int(vertices[vi].vx), int(vertices[vi].vy), int(vertices[vi].vz));

    pos *= (info.voxSideLen * .5f);
    pos += vpos;

    out_instId = instId;
    out_color = color;

    gl_Position = trans.proj * trans.view * vec4(pos, 1.0);
}