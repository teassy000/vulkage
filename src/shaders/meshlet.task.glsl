#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_mesh_shader: require
#extension GL_KHR_shader_subgroup_ballot: require

#extension GL_GOOGLE_include_directive: require

#include "mesh.h"

#define CULL 1

layout(local_size_x = TASKGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(binding = 1) readonly buffer Meshlets
{
    Meshlet meshlets[];
};

taskPayloadSharedEXT TaskPayload payload;


#if CULL
shared int sharedCount;
#endif


bool coneCull(vec4 cone, vec3 view)
{
    // alpha: angle between view and avgNorm 
    // theta: between avgNorm and cone boundary
    // pi/2 - alpha > theta
    return dot(view, cone.xyz) > cone.w; 
}


void main()
{
    uint mgi = gl_WorkGroupID.x ;
    uint ti = gl_LocalInvocationID.x;
    uint mi = mgi * TASKGP_SIZE + ti;

#if CULL
    sharedCount = 0;
    barrier();

    bool accept = !coneCull(meshlets[mi].cone, vec3(0, 0, 1));
    
    if(accept)
    {
        uint index = atomicAdd(sharedCount, 1);
        payload.meshletIndices[index] = mi;
    }

    
    EmitMeshTasksEXT(sharedCount, 1, 1);
#else
    payload.meshletIndices[ti] = mi;
    EmitMeshTasksEXT(TASKGP_SIZE, 1, 1);
#endif

}
