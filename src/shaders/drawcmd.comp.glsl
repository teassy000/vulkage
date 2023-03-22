#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require
#include "mesh.h"

layout(local_size_x = TASKGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block 
{
    MeshDrawCull cull;
};

layout(binding = 0) readonly buffer MeshDraws
{
    MeshDraw draws[];
};

layout(binding = 1) writeonly buffer DrawCommands
{
    MeshDrawCommand drawCmds[];
};  

layout(binding = 2) buffer DrawCommandCount
{
    uint drawCmdCount;
};

void main()
{
    uint ti = gl_LocalInvocationID.x;
    uint gi = gl_WorkGroupID.x;
    uint di = gi * TASKGP_SIZE + ti;

    // TODO: update camera pos so the frustum could always updated
    vec3 center = rotateQuat(draws[di].center, draws[di].orit) * draws[di].scale + draws[di].pos;
    float radius = draws[di].radius * draws[di].scale;

    bool visible = true;
    visible = visible && (center.z * cull.frustum[1] + abs(center.x) * cull.frustum[0] > -radius);
	visible = visible && (center.z * cull.frustum[3] + abs(center.y) * cull.frustum[2] > -radius);
	
    visible = visible && (center.z + radius > cull.znear);
    visible = visible && (center.z - radius < cull.zfar);

    //visible = visible && false;

    if(visible)
    {
        uint dci = atomicAdd(drawCmdCount, 1);

        drawCmds[dci].drawId = di;
        drawCmds[dci].indexCount = draws[di].indexCount;
        drawCmds[dci].instanceCount = 1;
        drawCmds[dci].firstIndex = draws[di].indexOffset;
        drawCmds[dci].vertexOffset = draws[di].vertexOffset;
        drawCmds[dci].firstInstance = 0;
        drawCmds[dci].local_x = (draws[di].meshletCount +63)/64; 
        drawCmds[dci].local_y = 1;
        drawCmds[dci].local_z = 1;
    }
}

