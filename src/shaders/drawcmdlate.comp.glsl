#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh.h"
#include "math.h"

layout(local_size_x = TASKGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block 
{
    MeshDrawCull cull;
};

layout(binding = 0) readonly buffer Meshes
{
    Mesh meshes[];
};

layout(binding = 1) readonly buffer MeshDraws
{
    MeshDraw draws[];
};

layout(binding = 2) readonly buffer Transform
{
    TransformData transform;
};

layout(binding = 3) writeonly buffer DrawCommands
{
    MeshDrawCommand drawCmds[];
};  

layout(binding = 4) buffer DrawCommandCount
{
    uint drawCmdCount;
};

layout(binding = 5) buffer DrawVisibility
{
    uint drawVisibility[];
};

layout(binding = 6) uniform sampler2D depthPyramid;

void main()
{
    uint di = gl_GlobalInvocationID.x;

    Mesh mesh = meshes[draws[di].meshIdx];

    vec4 center = transform.view * vec4(rotateQuat(mesh.center, draws[di].orit) * draws[di].scale + draws[di].pos, 1.0);
    float radius = mesh.radius * draws[di].scale;

    bool visible = true;
    visible = visible && (center.z * cull.frustum[1] + abs(center.x) * cull.frustum[0] > -radius);
	visible = visible && (center.z * cull.frustum[3] + abs(center.y) * cull.frustum[2] > -radius);
	
    visible = visible && (center.z + radius > cull.znear);
    visible = visible && (center.z - radius < cull.zfar);

    // visible = visible && false;
    visible = visible || (cull.enableCull == 0);




    drawVisibility[di] = visible ? 1 : 0;
}