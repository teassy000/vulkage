#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"
#include "rc_common2d.h"
#include "debug_gpu.h"

layout(constant_id = 0) const bool RAY = false;

layout(push_constant) uniform block
{
    Rc2dMergeData data;
};

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0, RGBA8) uniform image2DArray in_rc; // rc data
layout(binding = 1, RGBA8) uniform image2DArray in_baseRc; 
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

        const uint nextLvFactor = factor * 2;
        const ivec2 res = ivec2(data.rc.width, data.rc.height);

        const uint cn1_dRes = data.rc.c0_dRes * nextLvFactor;

        const ivec2 cn1_probeCount = res / ivec2(cn1_dRes);

        ProbeSamp cn0_samp = getProbSamp(di.xy, cn_dRes);

        const vec3 n0uv = vec3(vec2(di) / vec2(screenSize), float(currLv));

        ivec2 cn1_probeIdx = cn0_samp.baseIdx / 2;
        ivec2 cn1_subOffset = cn1_probeIdx % 2;
        ivec2 cn1_baseProbeIdx = cn1_probeIdx - cn1_subOffset;

        ivec2 cn_rPos = di.xy % ivec2(cn_dRes);
        uint cn_rIdx = uint(cn_rPos.x + cn_rPos.y * cn_dRes);
        uint cn1_rIdx = cn_rIdx * 4;

        // 4 probes
        vec4 colors[4];
        for (int jj = 0; jj < 4; ++jj)
        {
            ivec2 nextProbeIdx = cn1_baseProbeIdx + getOffsets(jj);
            vec4 mergedColor = vec4(0.f);
            
            // 4 rays
            for (int ii = 0; ii < 4; ++ii)
            {
                ivec2 texelPos = getTexelPos(nextProbeIdx, int(cn1_dRes), int(cn1_rIdx + ii));
                vec4 c = imageLoad(in_baseRc, ivec3(texelPos, currLv + 1));

                mergedColor += c;
            }
            mergedColor /= 4.f;
            colors[jj] = mergedColor;
        }

        ivec2 cn_suboff = cn0_samp.baseIdx % 2;
        vec2 cn1_ratio = getRatio(cn1_subOffset.x + cn1_subOffset.y * 2, cn_suboff.x + cn_suboff.y * 2);
        vec4 color0 = mix(colors[0], colors[1], cn1_ratio.x);
        vec4 color1 = mix(colors[2], colors[3], cn1_ratio.x);
        vec4 color = mix(color0, color1, cn1_ratio.y);

        ivec2 cn0_texelPos = getTexelPos(cn0_samp.baseIdx, int(cn_dRes), int(cn_rIdx));
        vec4 baseColor = imageLoad(in_rc, ivec3(cn0_texelPos, currLv));
        vec4 c = mergeIntervals(baseColor, color);

        imageStore(merged_rc, ivec3(di, currLv), c);
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
        imageStore(merged_rc, ivec3(di, 0), mergedRadiance);
    }
}