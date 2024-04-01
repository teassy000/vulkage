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
	outUVW = (inPos + vec3(1.0)) * 0.5;
	
	mat4 viewWithoutTranslation = mat4(trans.view[0], trans.view[1], trans.view[2], vec4(vec3(0.0), 1.0));

	gl_Position = trans.proj *viewWithoutTranslation * vec4(inPos, 1.0);
	outColor = vec3(1.0, 0.0, 0.0);
}