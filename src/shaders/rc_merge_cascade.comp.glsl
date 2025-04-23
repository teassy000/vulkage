#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
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

layout(binding = 0) uniform sampler2D in_skybox;
layout(binding = 1) uniform sampler2DArray in_rc; // rc data
layout(binding = 2) uniform sampler2DArray in_merged_rc; // intermediate data, which is merged [current level + 1] + [next level + 1] into the current level
layout(binding = 3, RGBA8) uniform image2DArray merged_rc; // store the merged data

vec4 MergeIntervals(vec4 _near, vec4 _far)
{
    return vec4(_near.rgb + _near.a * _far.rgb, _near.a * _far.a);
}

ivec3 getTrilinearProbeOffset(uint _idx)
{
    _idx = clamp(_idx, 0u, 7u);
    switch (_idx) {
        case 0u: return ivec3(0, 0, 0);
        case 1u: return ivec3(1, 0, 0);
        case 2u: return ivec3(0, 1, 0);
        case 3u: return ivec3(1, 1, 0);
        case 4u: return ivec3(0, 0, 1);
        case 5u: return ivec3(1, 0, 1);
        case 6u: return ivec3(0, 1, 1);
        case 7u: return ivec3(1, 1, 1);
    }
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

struct ProbeSample
{
    ivec3 baseIdx;
    vec3 ratio;
};

void trilinearWeights(vec3 _ratio, out float _weights[8] )
{
    _weights[0] = (1.0 - _ratio.x) * (1.0 - _ratio.y) * (1.0 - _ratio.z);
    _weights[1] =  _ratio.x * (1.0 - _ratio.y) * (1.0 - _ratio.z);
    _weights[2] = (1.0 - _ratio.x) * _ratio.y * (1.0 - _ratio.z);
    _weights[3] = _ratio.x* _ratio.y * (1.0 - _ratio.z);
    _weights[4] = (1.0 - _ratio.x) * (1.0 - _ratio.y) * _ratio.z;
    _weights[5] = _ratio.x * (1.0 - _ratio.y) * _ratio.z;
    _weights[6] = (1.0 - _ratio.x) * _ratio.y * _ratio.z;
    _weights[7] = _ratio.x * _ratio.y * _ratio.z;

    // no need to normalize weights, because the sum of weights is 1.0
    /*
    // normalize weights
    float sum = 0.0;
    for (uint ii = 0; ii < 8; ++ii)
    {
        sum += _weights[ii];
    }

    sum = sum < EPSILON ? 1.0 : sum; // avoid divide by zero

    for (uint ii = 0; ii < 8; ++ii)
    {
        _weights[ii] /= sum;
    }
    */
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


void main()
{
    ivec3 di = ivec3(gl_GlobalInvocationID.xyz);
    uint raySideCount = data.ray_sideCount;
    uint probeSideCount = data.probe_sideCount;
    uint currLv = data.currLv;
    uint layer = di.z;

    if (RAY_PRIME)
    {
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

        ivec2 currTexelPos = getRCTexelPos(data.idxType, raySideCount, probeSideCount, probeIdx, rayIdx);
        uint layerIdx = data.offset + layer;
        vec2 uv0 = vec2(currTexelPos + .5f) / float(probeSideCount * raySideCount); // 0-1 range
        vec4 radiance0 = vec4(0.f);

#if DEBUG_LEVELS
        radiance0 = getDebugLvColor(currLv);
#else
        radiance0 = texture(in_rc, vec3(uv0, layerIdx));  // current currLv would always use the in_rc value;
#endif

        ProbeSample probe_samp = getProbeNextLvSamp(ivec3(probeIdx.xy, float(layer)));

        vec4 colors[8];
        float weights[8];

        trilinearWeights(probe_samp.ratio, weights);

        uint nextProbeSideCount = probeSideCount >> 1;
        uint nextRaySideCount = raySideCount * 2;
        switch (data.idxType)
        {
            case 0: 
                // probe idx
                // neighboring texels are 2x2 of near rays
                // simply use bilinear filtering for 4 rays in one probe, and sperate sample between 8 probes

                // for 8 porbes of next level of cascade
                ivec2 nextTexelPos = ivec2(0u);
                for (uint ii = 0; ii < 8; ++ii)
                {
                    ivec3 offset = getTrilinearProbeOffset(ii);
                    nextTexelPos = getRCTexelPos(data.idxType, nextRaySideCount, nextProbeSideCount, probe_samp.baseIdx.xy + offset.xy, rayIdx);

                    vec2 uv = vec2(nextTexelPos + .5f) / float(probeSideCount * raySideCount);
                    ivec3 nextLvProbeIdx = getNextLvProbeIdx(di, nextProbeSideCount, offset);
                    uint nextLvLayerIdx = nextLvProbeIdx.z + data.offset + probeSideCount;

                    vec4 radianceN_1 = texture(in_merged_rc, vec3(uv, nextLvLayerIdx));
#if DEBUG_LEVELS
                    if (currLv == data.endLv - 1)
                    {
                        radianceN_1 = getDebugLvColor(currLv + 1) - vec4(0.05);
                    }
#endif // DEBUG_LEVELS

                    radiance0 += radianceN_1 * weights[ii];
                }

                break;
            case 1:
                // ray idx
                // neighboring texels are 2x2 of near probes
                // but it still requires bilinear filtering for cross layers, maybe 3D texture would help
                // simply use bilinear filtering 4 probes in one ray grid, and between 2 layers
                // then 4 times for 4 ray grid
                // for 2 layers
                //      for 4 ray grid
                for (uint ii = 0; ii < 8; ++ii)
                {
                    ivec3 offset = getTrilinearProbeOffset(ii);
                    currTexelPos = getRCTexelPos(data.idxType, nextRaySideCount, nextProbeSideCount, probe_samp.baseIdx.xy + offset.xy, rayIdx);
                    vec2 uv = vec2(nextTexelPos + .5f) / float(probeSideCount * raySideCount);

                    ivec3 nextLvProbeIdx = getNextLvProbeIdx(di, nextProbeSideCount, offset);
                    uint nextLvLayerIdx = nextLvProbeIdx.z + data.offset + probeSideCount;
                    vec4 radianceN_1 = texture(in_merged_rc, vec3(uv, nextLvLayerIdx));
                    colors[ii] = radianceN_1;
                    
#if DEBUG_LEVELS
                    if (currLv == data.endLv - 1)
                    {
                        radianceN_1 = getDebugLvColor(currLv + 1) - vec4(0.05);
                    }
#endif // DEBUG_LEVELS

                    radiance0 += radianceN_1 * weights[ii];
                }

                break;
        }

        imageStore(merged_rc, ivec3(currTexelPos, layerIdx), radiance0);
    }
    else
    {
        if (di.x >= probeSideCount || di.y >= probeSideCount || di.z >= probeSideCount)
            return;

        vec4 mergedRadiance = vec4(0.f, 0.f, 0.f, 1.f);
        for (uint hh = 0; hh < raySideCount; ++hh)
        {
            for (uint ww = 0; ww < raySideCount; ++ww)
            {
                vec2 texelPos = getRCTexelPos(data.idxType, raySideCount, probeSideCount, di.xy, ivec2(ww, hh));

                vec2 uv = vec2(texelPos + .5f) / float(probeSideCount * raySideCount);
                vec4 radiance = texture(in_rc, vec3(uv.xy, di.z));

                if (radiance.w < EPSILON)
                    continue;

                mergedRadiance.xyz += radiance.xyz;
            }
        }

        mergedRadiance /= float(raySideCount * raySideCount);

        imageStore(merged_rc, ivec3(di.xyz), mergedRadiance);
    }
}