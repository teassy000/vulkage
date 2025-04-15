#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "rc_common.h"
#include "debug_gpu.h"

layout(push_constant) uniform block
{
    RCMergeData data;
};

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D in_skybox;
layout(binding = 1, RGBA8) uniform readonly image2DArray in_rc; // radiance cascade
layout(binding = 2, RGBA8) uniform writeonly image2DArray out_merged_rc;


vec2 getTexelPos(uint _idxType, uint _raySideCount, uint _probSideCount, ivec2 _probIdx, ivec2 _rayIdx)
{
    ivec2 texelPos = ivec2(0);
    switch (_idxType) {
        case 0:  // probe first index
            texelPos = _probIdx.xy * int(_raySideCount) + _rayIdx;
            break;
        case 1: // ray first index
            texelPos = _rayIdx * int(_probSideCount) + _probIdx.xy;
            break;
    }

    return texelPos;
}

void main()
{
    ivec3 di = ivec3(gl_GlobalInvocationID.xyz);
    
    uint raySideCount = data.ray_sideCount;
    uint probeSideCount = data.probe_sideCount;

    if (di.x >= probeSideCount || di.y >= probeSideCount || di.z >= probeSideCount)
        return;

    vec4 mergedRadiance = vec4(0.f);
    for (uint hh = 0; hh < raySideCount; ++hh) {
        for (uint ww = 0; ww < raySideCount; ++ww) {
            vec2 texelPos = getTexelPos(0, raySideCount, probeSideCount, di.xy, ivec2(ww, hh));

            vec4 radiance = imageLoad(in_rc, ivec3(texelPos, di.z));

            if (radiance.w < EPSILON)
                continue;

            mergedRadiance += radiance;
        }
    }

    mergedRadiance /= float(raySideCount * raySideCount);
    imageStore(out_merged_rc, ivec3(di.xyz), mergedRadiance);
}