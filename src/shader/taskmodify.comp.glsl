#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#extension GL_EXT_null_initializer: require


#include "mesh_gpu.h"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) buffer CommandCount
{
	uint cmdCount;
	uint groupCountX;
	uint groupCountY;
	uint groupCountZ;
};


layout(binding = 1) writeonly buffer TaskCommands
{
    MeshTaskCommand taskCmds[];
};

void main()
{
    uint id = gl_LocalInvocationID.x;
    uint count = cmdCount;
    
    if(id == 0)
    {
        groupCountX = min((count + 63) / 64, 65535);
        groupCountY = 64;
        groupCountZ = 1;
    }

    // fix round issue
    uint boundary = (count + 63) & ~63;
    MeshTaskCommand dummy = {};

    if(count + id < boundary)
        taskCmds[count + id] = dummy;
}