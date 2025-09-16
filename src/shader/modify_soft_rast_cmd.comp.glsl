#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#extension GL_EXT_null_initializer: require


#include "mesh_gpu.h"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block 
{
    uvec2 res;
};

layout(binding = 0) buffer CommandCount
{
	uint cmdCount;
	uint groupCountX;
	uint groupCountY;
	uint groupCountZ;
};


void main()
{
    uint id = gl_LocalInvocationID.x;
    uint count = cmdCount;
    
    if(id == 0)
    {
        groupCountX = res.x;
        groupCountY = res.y;
        groupCountZ = count;
    }
}