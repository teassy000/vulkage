#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"
#include "rc_common.h"

layout(local_size_x = 32, local_size_y = 32, local_size_z = 32) in;

void main()
{

}