#version 450
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require
#include "mesh.h"

layout(local_size_x = TASKGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) readonly buffer MeshDraws
{
    MeshDraw draws[];
};

layout(binding = 1) writeonly buffer DrawCommands
{
    MeshDrawCommand drawCmds[];
};  

void main()
{
    uint ti = gl_LocalInvocationID.x;
    uint gi = gl_WorkGroupID.x;
    uint di = gi * TASKGP_SIZE + ti;

    drawCmds[di].indexCount = draws[di].indexCount;

    drawCmds[di].instanceCount = 1;
    drawCmds[di].firstIndex = draws[di].indexOffset;
    drawCmds[di].vertexOffset = draws[di].vertexOffset;
    drawCmds[di].firstInstance = 0;
    
    drawCmds[di].local_x = uint(draws[di].meshletCount + 31 / 32);
    drawCmds[di].local_y = 1;
    drawCmds[di].local_z = 1;
}

