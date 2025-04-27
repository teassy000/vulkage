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

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;


layout(binding = 0, RGBA8) uniform image2DArray out_rc; // store the merged data


// merge intervals based on the alpha channel
// a = 0.0 means hitted
// a = 1.0 means not hitted
vec4 mergeIntervals(vec4 _near, vec4 _far)
{
    return vec4(_near.rgb + _near.a * _far.rgb, _near.a * _far.a);
}

void main()
{
    const ivec3 di = ivec3(gl_GlobalInvocationID.xyz);
    imageStore(out_rc, ivec3(di.xyz), vec4(1.f));
}