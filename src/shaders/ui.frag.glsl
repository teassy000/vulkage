#version 450

layout (binding = 0) uniform sampler2D fontSampler;
layout (binding = 1) uniform sampler2DArray dummy;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main() 
{
	outColor = inColor * texture(fontSampler, inUV);
}