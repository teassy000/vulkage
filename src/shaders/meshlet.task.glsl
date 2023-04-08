#version 450

#extension GL_ARB_shader_draw_parameters: require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_mesh_shader: require
#extension GL_KHR_shader_subgroup_ballot: require


#extension GL_GOOGLE_include_directive: require

#include "mesh.h"
#include "math.h"

#define CULL 1

layout(local_size_x = TASKGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block 
{
    Globals globals;
};


layout(binding = 0) readonly buffer DrawCommands 
{
    MeshDrawCommand drawCmds[];
};

layout(binding = 1) readonly buffer Meshes
{
    Mesh meshes[];
};

layout(binding = 2) readonly buffer MeshDraws
{
    MeshDraw meshDraws[];
};

layout(binding = 3) readonly buffer Meshlets
{
    Meshlet meshlets[];
};


taskPayloadSharedEXT TaskPayload payload;


#if CULL
shared int sharedCount;
#endif

bool coneCullApex(vec3 cone_apex, vec3 cone_axis, float cone_cutoff, vec3 camera_position)
{
    return dot(normalize(cone_apex - camera_position), cone_axis) >= cone_cutoff;
}

void main()
{
    uint drawId = drawCmds[gl_DrawIDARB].drawId;
    uint lodIdx = drawCmds[gl_DrawIDARB].lodIdx;
    MeshDraw meshDraw = meshDraws[drawId];
    Mesh mesh = meshes[meshDraw.meshIdx];
    MeshLod lod = mesh.lods[lodIdx];

    uint mgi = gl_WorkGroupID.x;
    uint ti = gl_LocalInvocationID.x;
    uint mi = mgi * TASKGP_SIZE + ti + lod.meshletOffset;
    uint mLocalId = mgi * TASKGP_SIZE + ti;

#if CULL
    vec3 axit = vec3( int(meshlets[mi].cone_axis[0]) / 127.0, int(meshlets[mi].cone_axis[1]) / 127.0, int(meshlets[mi].cone_axis[2]) / 127.0); 
    vec3 cone_axis = rotateQuat(axit, meshDraw.orit);
    vec3 cone_center = rotateQuat(meshlets[mi].center, meshDraw.orit) * meshDraw.scale + meshDraw.pos;
    float cone_radians = meshlets[mi].radians;
    float cone_cutoff = int(meshlets[mi].cone_cutoff) / 127.0;
    vec3 cameraPos = globals.cameraPos;
    sharedCount = 0;
    
    barrier();

    bool accept = !coneCull(cone_center, cone_radians, cone_axis, cone_cutoff, cameraPos);
    accept = (accept == mLocalId < lod.meshletCount);

    if(accept)
    {
        uint index = atomicAdd(sharedCount, 1);
        payload.meshletIndices[index] = mi;
    }
    barrier(); 
    payload.drawId = drawId;

    // TODO: figure out correct task count to prevent overdraw
    EmitMeshTasksEXT(sharedCount, 1, 1);
#else
    payload.meshletIndices[ti] = mi;

    payload.drawId = drawId;

    uint emitCount = min(TASKGP_SIZE, lod.meshletCount - mLocalId );
    EmitMeshTasksEXT(emitCount, 1, 1);
#endif

}
