#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_mesh_shader: require

#extension GL_GOOGLE_include_directive: require

#include "mesh.h"


layout(local_size_x = TASK_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(binding = 1) readonly buffer Meshlets
{
    Meshlet meshlets[];
};

taskPayloadSharedEXT TaskPayload payload;

void main()
{
    uint mi = gl_WorkGroupID.x;
    uint ti = gl_LocalInvocationID.x;

    if(ti == 0) {
        payload.offset = mi * gl_WorkGroupSize.x;   
        EmitMeshTasksEXT(TASK_SIZE, 1, 1);
    }

}
