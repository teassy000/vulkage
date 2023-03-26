#version 450

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f) uniform writeonly image2D iout;

void main()
{
	uvec2 pos = gl_GlobalInvocationID.xy;

	imageStore(iout, ivec2(pos), vec4(0));
}