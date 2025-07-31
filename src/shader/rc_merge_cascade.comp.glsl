#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"
#include "rc_common.h"
#include "debug_gpu.h"

#define DEBUG_LEVELS 0

// there's 2 types of merge:
// 1. merge all cascades into the level 0, can be understood as merge rays along the ray direction, cross all cascades
// 2. merge each probe(cascade interval) into 1 texel, which can be treated as a LoD of the level 0
layout(constant_id = 0) const bool RAY_PRIME = false;

layout(push_constant) uniform block
{
    RCMergeData data;
};

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(binding = 0) uniform samplerCube in_skybox;
layout(binding = 1) uniform sampler2DArray in_rc; // rc data
layout(binding = 2) uniform sampler2DArray in_merged_rc; // intermediate data, which is merged [current level + 1] + [next level + 1] into the current level
layout(binding = 3, RGBA8) uniform image2DArray merged_rc; // store the merged data


// merge intervals based on the alpha channel
// a = 0.0 means hitted
// a = 1.0 means not hitted
vec4 mergeIntervals(vec4 _near, vec4 _far)
{
    return vec4(_near.rgb + _near.a * _far.rgb, _near.a * _far.a);
}


ivec2 getBilinearRayOffset(uint _idx)
{
    _idx = clamp(_idx, 0u, 4u);
    switch (_idx)
    {
        case 0u: return ivec2(0, 0);
        case 1u: return ivec2(1, 0);
        case 2u: return ivec2(0, 1);
        case 3u: return ivec2(1, 1);
    }
} 


// find the index of current probe in the 8 * 8 * 8 grid
ProbeSample getProbeNextLvSamp(ivec3 _probeIdx)
{
    // get the probe index in the 8 * 8 * 8 grid
    ivec3 nextIdx = _probeIdx / 2;
    ivec3 probeIdx = _probeIdx - nextIdx * 2;

    float step = 1.f / 8.f;

    ProbeSample samp;
    samp.baseIdx = nextIdx;
    samp.ratio = step * (probeIdx + 1);

    return samp;
}

vec4 getDebugLvColor(uint _lv)
{
    vec4 c = vec4(0.0f);
    switch (_lv)
    {
        case 0:
            c = vec4(1.0f, 0.f, 0.f, 1.f);
            break;
        case 1:
            c = vec4(0.0f, 1.f, 0.f, 1.f);
            break;
        case 2:
            c = vec4(0.0f, 0.f, 1.f, 1.f);
            break;
        case 3:
            c = vec4(0.5f, 0.f, 1.f, 1.f);
            break;
        case 4:
            c = vec4(1.0f, 1.f, 0.f, 1.f);
            break;
        case 5:
            c = vec4(0.f, 1.f, 1.f, 1.f);
            break;
        case 6:
            c = vec4(1.f, 0.f, 1.f, 1.f);
            break;
    }
    return c;
}


vec3 dirToCubemapUV(vec3 _dir)
{
    vec3 uv = vec3(0.f);
    float absX = abs(_dir.x);
    float absY = abs(_dir.y);
    float absZ = abs(_dir.z);
    if (absX >= absY && absX >= absZ)
    {
        uv.x = (_dir.x > 0.f) ? _dir.z : -_dir.z;
        uv.y = _dir.y;
        uv.z = _dir.x;
    }
    else if (absY >= absX && absY >= absZ)
    {
        uv.x = _dir.x;
        uv.y = (_dir.y > 0.f) ? -_dir.z : _dir.z;
        uv.z = _dir.y;
    }
    else
    {
        uv.x = _dir.x;
        uv.y = _dir.y;
        uv.z = (_dir.z > 0.f) ? -_dir.y : _dir.y;
    }
    return uv * .5f + .5f; // map to [0, 1]
}


void main()
{
    const ivec3 di = ivec3(gl_GlobalInvocationID.xyz);
    const uint raySideCount = data.ray_sideCount;
    const uint probeSideCount = data.probe_sideCount;
    const uint currLv = data.currLv;
    const uint layer = di.z;

    if (RAY_PRIME)
    {
        uint pixelCount = raySideCount * probeSideCount;
        if ((di.z > probeSideCount - 1) || (di.x > pixelCount - 1) || (di.y > pixelCount)) {

            return;
        }

        // sample 8 probes for each ray, weighted average
        // but for 2 types of idx, there's are 2 different ways to sample (with optimize) to take advantage of hardware bilinear filtering
        // 1. probe idx:
        //   neighboring texels are 2x2 of near rays
        // 2. ray idx:
        //      neighboring texels are 2x2 of near probes
        //      but it still requires bilinear filtering for cross layers, maybe 3D texture would help
        ivec2 rayIdx = ivec2(0u);
        ivec2 probeIdx = ivec2(0u);

        ivec2 c0_rayIdx = ivec2(0u);
        ivec2 c0_probeIdx = ivec2(0u);
        switch (data.idxType) { 
            case 0: // probe idx
                probeIdx = ivec2(di.xy / raySideCount);
                rayIdx = ivec2(di.xy % raySideCount);
                c0_probeIdx = ivec2(di.xy / data.c0_raySideCount);
                c0_rayIdx = ivec2(di.xy % data.c0_raySideCount);
                break;
            case 1: // ray idx
                probeIdx = ivec2(di.xy % probeSideCount);
                rayIdx = ivec2(di.xy / probeSideCount);
                c0_probeIdx = ivec2(di.xy % data.c0_probeSideCount);
                c0_rayIdx = ivec2(di.xy / data.c0_probeSideCount);
                break;
        }

        const ivec2 currTexelPos = getRCTexelPos(data.idxType, raySideCount, probeSideCount, probeIdx, rayIdx);
        uint layerIdx = data.offset + layer;
        vec2 uv0 = vec2(currTexelPos + .5f) / float(probeSideCount * raySideCount); // 0-1 range
        vec4 radiance0 = vec4(0.f);

#if DEBUG_LEVELS
        radiance0 = getDebugLvColor(currLv);
#else
        radiance0 = texture(in_rc, vec3(uv0, layerIdx));  // current currLv would always use the in_rc value;
#endif

        ProbeSample probe_samp = getProbeNextLvSamp(ivec3(probeIdx.xy, float(layer)));

        float weights[8];
        trilinearWeights(probe_samp.ratio, weights);

        const uint nextProbeSideCount = probeSideCount >> 1;
        const uint nextRaySideCount = raySideCount * 2;
        const ivec2 nextRayIdx = rayIdx * 2;
        // ==========================================================================================
        // A === probe idx
        // ==========================================================================================
        // neighboring texels are 2x2 of near rays
        // simply use bilinear filtering for 4 rays in one probe, and sperate sample between 8 probes
        // for 8 porbes of next level of cascades
        // ==========================================================================================
        // B === ray idx
        // ==========================================================================================
        // neighboring texels are 2x2 of near probes
        // but it still requires bilinear filtering for cross layers
        // simply use bilinear filtering 4 probes in one ray grid, and between 2 layers
        // then 4 times for 4 ray grid
        // for 2 layers
        //      for 4 ray grid
        // thus using 8 samples for 2 layers
        // ==========================================================================================
        vec4 mergedRadiance = vec4(0.f);
        for (uint ii = 0; ii < 8; ++ii)
        {
            ivec3 offset = getTrilinearProbeOffset(ii);
            ivec2 nextTexelPos = getRCTexelPos(data.idxType, nextRaySideCount, nextProbeSideCount, probe_samp.baseIdx.xy + offset.xy, nextRayIdx);

            vec2 uv = vec2(nextTexelPos + .5f) / float(nextProbeSideCount * nextRaySideCount);
            ivec3 nextLvProbeIdx = getNextLvProbeIdx(di, nextProbeSideCount, offset);
            uint nextLvLayerIdx = nextLvProbeIdx.z + data.offset + probeSideCount;

            vec4 radianceN_1 = texture(in_merged_rc, vec3(uv, nextLvLayerIdx));
#if DEBUG_LEVELS
            if (currLv == data.endLv - 1)
            {
                radianceN_1 = getDebugLvColor(currLv + 1) - vec4(0.05);
            }
#endif // DEBUG_LEVELS

            if ((currLv == data.endLv - 1) && compare(radianceN_1.a, 1.f))
            {
                vec2 rayUV = (vec2(nextRayIdx) + .5f) / float(nextRaySideCount);
                rayUV = rayUV * 2.f - 1.f; // to [-1.f, 1.f]
                vec3 skyUV = dirToCubemapUV(oct_to_float32x3(rayUV));
                radianceN_1 = texture(in_skybox, skyUV);
            }

            mergedRadiance += mergeIntervals(radiance0, radianceN_1) * weights[ii];
        }

        imageStore(merged_rc, ivec3(currTexelPos, layerIdx), mergedRadiance);
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
                vec2 texelPos = getRCTexelPos(data.idxType, raySideCount, probeSideCount, di.xy, ivec2(ww, hh));

                vec2 uv = vec2(texelPos + .5f) / float(probeSideCount * raySideCount);
                vec4 radiance = texture(in_rc, vec3(uv.xy, di.z));

                if (compare(radiance.w, 1.f))
                    continue;

                mergedRadiance.rgb += radiance.rgb;
            }
        }

        mergedRadiance /= float(raySideCount * raySideCount);

        imageStore(merged_rc, ivec3(di.xyz), mergedRadiance);
    }
}