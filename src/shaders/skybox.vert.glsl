#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require
#include "mesh_gpu.h"


layout (location = 0) in vec3 inPos;

layout (binding = 0) readonly buffer Transform
{
	TransformData trans;
};

layout (location = 0) out vec3 outUVW;
layout (location = 1) out vec3 outColor;

void main() 
{
	outUVW = (inPos + vec3(1.0))/2.0;
	outUVW.xy *= -1.0;
	gl_Position = trans.proj * trans.view * vec4(inPos.xyz, 1.0);
	outColor = vec3(1.0, 0.0, 0.0);
}