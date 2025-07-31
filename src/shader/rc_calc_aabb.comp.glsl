#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"

layout(local_size_x = TASKGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(constant_id = 0) const bool LATE = false;
layout(constant_id = 1) const bool TASK = false;
layout(constant_id = 2) const bool ALPHA_PASS = false;

// readonly
layout(binding = 0) readonly buffer Meshes
{
    Mesh meshes[];
};

layout(binding = 1) readonly buffer MeshDraws
{
    MeshDraw draws[];
};

// writeonly
layout(binding = 2) writeonly buffer DrawCommands
{
    MeshDrawCommand drawCmds[];
};

void main()
{
    uint di = gl_GlobalInvocationID.x;
    MeshDraw draw = draws[di];
    Mesh mesh = meshes[draw.meshIdx];

    MeshLod lod = mesh.lods[0];

}
