#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"
#include "rc_common2d.h"
#include "debug_gpu.h"
#include "pbr.h"

layout(constant_id = 0) const bool RAY = false;
layout(constant_id = 1) const bool BILINER_RING_FIX = true;

layout(push_constant) uniform block
{
    Rc2dMergeData data;
};

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0, RGBA8) uniform image2DArray in_rc; // rc data
layout(binding = 1, RGBA8) uniform image2DArray in_merged_rc; 
layout(binding = 2, RGBA8) uniform image2DArray merged_rc; // store the merged data


// merge intervals based on the alpha channel
// a = 0.0 means hitted
// a = 1.0 means not hitted
vec4 mergeIntervals(vec4 _near, vec4 _far)
{
    return vec4(_near.rgb + _near.a * _far.rgb, _near.a * _far.a);
}

ivec2 getTexelPos(ivec2 _probeIdx, int _dRes, int _rayIdx)
{
    ivec2 rayIDx = ivec2(_rayIdx % _dRes, _rayIdx / _dRes);
    ivec2 res = _probeIdx * _dRes + rayIDx;

    ivec2 newIdx = res / _dRes;

    // shift to the next line
    if (newIdx.x != _probeIdx.x)
    {
        res.x -= _dRes;
        res.y += 1;
        newIdx = res / _dRes;
    }
    // shift back to the start of probe
    if (newIdx.y != _probeIdx.y)
    {
        res.y -= _dRes;
    }

    return res;
}

vec2 getRatio(uint _cn1_off, uint _cn_off)
{
    vec2 ratio = vec2(0.f);
    switch (_cn1_off)
    {
        case 0: ratio = vec2(0.25, 0.25); break;
        case 1: ratio = vec2(0.75, 0.25); break;
        case 2: ratio = vec2(0.25, 0.75); break;
        case 3: ratio = vec2(0.75, 0.75); break;
    }

    switch (_cn_off)
    {
        case 0: ratio += vec2(-0.125, -0.125); break;
        case 1: ratio += vec2(0.125, -0.125); break;
        case 2: ratio += vec2(-0.125, 0.125); break;
        case 3: ratio += vec2(0.125, 0.125); break;
    }

    return ratio;
}

void main()
{
    const ivec2 di = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 screenSize = ivec2(data.rc.width, data.rc.height);

    if (RAY)
    {
        const uint currLv = data.lv;
        const uint factor = 1 << currLv;
        const uint cn_dRes = data.rc.c0_dRes * factor;
        const ivec2 mousePos = ivec2(data.rc.mpx, data.rc.mpy);

        const uint nextLvFactor = factor * 2;
        const ivec2 res = ivec2(data.rc.width, data.rc.height);

        const uint cn1_dRes = data.rc.c0_dRes * nextLvFactor;

        const ivec2 cn1_probeCount = res / ivec2(cn1_dRes);

        ProbeSamp cn0_samp = getProbSamp(di.xy, cn_dRes);

        const vec3 n0uv = vec3(vec2(di) / vec2(screenSize), float(currLv));
        // the n+1 lv offset based on the n lv:
        // only nearby n+1 probes counted
        // as a observe: it always the opposite direction of the n lv
        // e.g.: n: (0, 0) n+1: (1, 1), or n: (0, 1), n+1 (1, 0)
        ivec2 cn_suboff = cn0_samp.baseIdx % 2;
        ivec2 cn1_suboff = ivec2(1, 1) - cn_suboff;
        ivec2 cn1_probeIdx = cn0_samp.baseIdx / 2;
        ivec2 cn1_baseProbeIdx = cn1_probeIdx - cn1_suboff;

        vec2 cn1_ratio = getRatio(cn1_suboff.x + cn1_suboff.y * 2, cn_suboff.x + cn_suboff.y * 2);

        ivec2 cn_rPos = di.xy % ivec2(cn_dRes); // this only work when rays in same probe are nearby in the data
        uint cn_rIdx = uint(cn_rPos.x + cn_rPos.y * cn_dRes);
        uint cn1_rIdx = cn_rIdx * 4;

        vec4 outColor = vec4(0.f);

        if (BILINER_RING_FIX)
        {
            vec2 cn_probeCenter = (vec2(cn0_samp.baseIdx) + vec2(.5f)) * float(cn_dRes);

            // 4 rays
            vec4 mergedColor = vec4(0.f);
            for (int jj = 0; jj < 4; ++jj)
            {
                uint rayIdx = cn1_rIdx + jj;
                ivec2 rayIdxPos = ivec2(rayIdx % cn1_dRes, rayIdx / cn1_dRes);
                float rad = pixelToRadius(rayIdxPos, cn1_dRes);

                // 4 probes
                vec4 mergedColors[4];
                for (int ii = 0; ii < 4; ++ii)
                {
                    ivec2 nextProbeIdx = cn1_baseProbeIdx + getOffsets(ii);
                    ivec2 texelPos = getTexelPos(nextProbeIdx, int(cn1_dRes), int(cn1_rIdx + jj));
                    // N+1 traced color
                    vec4 col_n1 = imageLoad(in_merged_rc, ivec3(texelPos, currLv + 1));

                    // trace the ray from the N with ends
                    float c0Len = data.rc.c0_rLen;
                    float tmin = currLv + 1 == 0 ? 0.f : c0Len * float(1 << 2 * (currLv));
                    float tmax = c0Len * float(1 << 2 * (currLv + 1));
                    
                    vec2 cn1_probeCenter = (vec2(nextProbeIdx) + vec2(.5f)) * float(cn1_dRes);
                    vec2 endPos = cn1_probeCenter + vec2(cos(rad), sin(rad)) * float(tmax);
                    vec2 rayDir = normalize(endPos - cn_probeCenter);

                    Ray ray;
                    ray.origin = vec2(cn_probeCenter);
                    ray.dir = rayDir;

                    vec4 c_n0 = traceRay(ray, tmin, tmax, vec2(res), vec2(mousePos));
                    mergedColors[ii] = mergeIntervals(c_n0, col_n1);
                }

                vec4 color0 = mix(mergedColors[0], mergedColors[1], cn1_ratio.x);
                vec4 color1 = mix(mergedColors[2], mergedColors[3], cn1_ratio.x);
                vec4 color = mix(color0, color1, cn1_ratio.y);
                mergedColor += color;
            }
            outColor = mergedColor / 4.f;
        }
        else
        { // NO BILINER_RING_FIX
            ivec2 cn0_texelPos = getTexelPos(cn0_samp.baseIdx, int(cn_dRes), int(cn_rIdx));
            vec4 baseColor = imageLoad(in_rc, ivec3(cn0_texelPos, currLv));
            // 4 rays
            vec4 mergedColor = vec4(0.f);
            for (int jj = 0; jj < 4; ++jj)
            {
                // 4 probes
                vec4 colors[4];
                for (int ii = 0; ii < 4; ++ii)
                {
                    ivec2 nextProbeIdx = cn1_baseProbeIdx + getOffsets(ii);
                    ivec2 texelPos = getTexelPos(nextProbeIdx, int(cn1_dRes), int(cn1_rIdx + jj));
                    vec4 c = imageLoad(in_merged_rc, ivec3(texelPos, currLv + 1));

                    colors[ii] = c;
                }
                vec4 color0 = mix(colors[0], colors[1], cn1_ratio.x);
                vec4 color1 = mix(colors[2], colors[3], cn1_ratio.x);
                vec4 color = mix(color0, color1, cn1_ratio.y);

                outColor = mergeIntervals(baseColor, color);

                mergedColor += outColor;
            }
            outColor = mergedColor / 4.f;
        }

        imageStore(merged_rc, ivec3(di, currLv), outColor);
    }
    else
    {
        const uint c0_dRes = data.rc.c0_dRes;

        vec4 mergedRadiance = vec4(0.f);
        uint c0_dCount = c0_dRes * c0_dRes;
        for (uint ii = 0; ii < c0_dCount; ++ii)
        {
            ivec2 texelPos = getTexelPos(di.xy, int(c0_dRes), int(ii));

            if (texelPos.x > screenSize.x || texelPos.y > screenSize.y)
                continue;
            
            vec4 c = imageLoad(in_rc, ivec3(texelPos, 0));
            mergedRadiance += c;
        }

        mergedRadiance /= float(c0_dCount);

        mergedRadiance.rgb = OECF_sRGBFast(mergedRadiance.rgb);

        imageStore(merged_rc, ivec3(di, 0), mergedRadiance);
    }
}