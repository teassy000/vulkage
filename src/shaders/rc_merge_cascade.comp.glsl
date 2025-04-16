#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "rc_common.h"
#include "debug_gpu.h"

// there's 2 types of merge:
// 1. merge all cascades into the level 0, can be understood as merge rays along the ray direction, cross all cascades
// 2. merge each probe(cascade interval) into 1 texel, which can be treated as a LoD of the level 0
layout(constant_id = 0) const bool RAY_PRIME = false;

layout(push_constant) uniform block
{
    RCMergeData data;
};

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(binding = 0) uniform sampler2D in_skybox;
layout(binding = 1, RGBA8) uniform readonly image2DArray in_rc; // radiance cascade 
layout(binding = 2, RGBA8) uniform writeonly image2DArray out_merged_rc;

void main()
{
    ivec3 di = ivec3(gl_GlobalInvocationID.xyz);
    uint raySideCount = data.ray_sideCount;
    uint probeSideCount = data.probe_sideCount;

    if (RAY_PRIME)
    {
        imageStore(out_merged_rc, ivec3(di.xyz), vec4(1.f));
    }
    else
    {
        if (di.x >= probeSideCount || di.y >= probeSideCount || di.z >= probeSideCount)
            return;

        vec4 mergedRadiance = vec4(0.f);
        for (uint hh = 0; hh < raySideCount; ++hh)
        {
            for (uint ww = 0; ww < raySideCount; ++ww)
            {
                vec2 texelPos = getRCTexelPos(0, raySideCount, probeSideCount, di.xy, ivec2(ww, hh));

                vec4 radiance = imageLoad(in_rc, ivec3(texelPos, di.z));

                if (radiance.w < EPSILON)
                    continue;

                mergedRadiance += radiance;
            }
        }

        mergedRadiance /= float(raySideCount * raySideCount);
        imageStore(out_merged_rc, ivec3(di.xyz), mergedRadiance);
    }
}