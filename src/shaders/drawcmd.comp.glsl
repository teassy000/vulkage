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


    drawCmds[di].indexCount = draws[di].indexCount;
    drawCmds[di].instanceCount = visible ? 1 : 0;
    drawCmds[di].firstIndex = draws[di].indexOffset;
    drawCmds[di].vertexOffset = draws[di].vertexOffset;
    drawCmds[di].firstInstance = 0;
    
    drawCmds[di].local_x = visible ? ((draws[di].meshletCount +31)/32): 0; 
    drawCmds[di].local_y = 1;
    drawCmds[di].local_z = 1;
}

