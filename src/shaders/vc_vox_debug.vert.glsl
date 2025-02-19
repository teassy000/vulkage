#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "rc_common.h"


layout(location = 0) in vec3 inPos;

layout(binding = 0) readonly uniform Transform
{
    TransformData trans;
};

void main()
{
    gl_Position = trans.proj * trans.view * vec4(inPos, 1.0);
}