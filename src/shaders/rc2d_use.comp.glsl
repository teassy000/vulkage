#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "rc_common2d.h"
#include "debug_gpu.h"


layout(push_constant) uniform block
{
    Rc2dData data;
};

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform sampler2DArray in_merged_rc; // intermediate data, which is merged [current level + 1] + [next level + 1] into the current level
layout(binding = 1, RGBA8) uniform image2D rt; // store the merged data


void main()
{
    const ivec3 di = ivec3(gl_GlobalInvocationID.xyz);

    vec3 color = vec3(0.f);
    vec2 uv = di.xy / vec2(data.width, data.height);
    vec4 sd = sdf(vec2(di.xy), vec2(data.width, data.height));
    if (sd.w < 0.5)
        color = sd.rgb;


    imageStore(rt, ivec2(di.xy), vec4(color, 1.f));
}