#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout (push_constant) uniform blocks {
	vec2 imageSize;
};

layout(binding = 0) uniform sampler2D inSampler;
layout(binding = 1, r32f) uniform writeonly image2D outImg;

void main()
{
	uvec2 pos = gl_GlobalInvocationID.xy;

	float depth  = texture(inSampler, (vec2(pos) + vec2(0.5)) / imageSize).x;

	imageStore(outImg, ivec2(pos), vec4(depth, 0, 0, 1));
}