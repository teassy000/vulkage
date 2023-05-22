#version 450

layout (binding = 1) uniform samplerCube skyboxSamp;

layout (location = 0) in vec3 inUVW;
layout (location = 1) in vec3 inColor;

layout (location = 0) out vec4 outColor;

void main() 
{
	vec3 color = texture(skyboxSamp, inUVW).rgb;
	outColor = vec4(color, 1.0);
}