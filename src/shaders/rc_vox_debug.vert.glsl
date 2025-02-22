#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_ARB_draw_instanced : require
//#extension GL_ARB_shader_draw_parameters : require

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

layout(binding = 2) readonly buffer DrawCommand
{
    MeshDrawCommand drawCmds [];
};

layout(binding = 3) readonly buffer WorldPos
{
    vec3 drawPositions [];
};

layout(location = 0) out flat uint out_instId;

void main()
{
    int instId = gl_InstanceIndex;
    vec3 vpos = drawPositions[instId];

    uint vi = gl_VertexIndex;
    vec3 pos = vec3(int(vertices[vi].vx), int(vertices[vi].vy), int(vertices[vi].vz));

    pos *= (info.voxSideLen * .5f);
    pos += vpos;

    out_instId = instId;
    // no view since the view already applied in the command gen pass
    gl_Position = trans.proj * trans.view * vec4(pos, 1.0);
}