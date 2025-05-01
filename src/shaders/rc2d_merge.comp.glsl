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

void main()
{
    const ivec2 di = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 screenSize = ivec2(data.rc.width, data.rc.height);

    if (RAY)
    {
        const uint currLv = data.lv;
        const uint factor = 1 << currLv;
        const uint nextLvFactor = factor * 2;
        const ivec2 res = ivec2(data.rc.width, data.rc.height);

        const uint cn_dRes = data.rc.c0_dRes * factor;
        const uint cn1_dRes = data.rc.c0_dRes * nextLvFactor;

        const ivec2 cn1_probeCount = res / ivec2(cn1_dRes);

        ProbeSamp cn0_samp = getProbSamp(di.xy, cn_dRes);
        ProbeSamp cn1_samp = getNextLvProbeSamp(cn0_samp);

        vec4 weights = getWeights(cn0_samp.ratio);

        const vec3 n0uv = vec3(vec2(di) / vec2(screenSize), float(currLv));
        vec4 baseColor = imageLoad(in_rc, ivec3(di, currLv));

        ivec2 cn1_probeIdx = cn0_samp.baseIdx / 2;
        ivec2 cn1_subOffset = cn0_samp.baseIdx % 2;
        ivec2 cn1_baseProbeIdx = cn1_probeIdx - cn1_subOffset;
        vec2 cn1_fract = fract(vec2(cn0_samp.baseIdx) / vec2(cn1_probeCount));

        ivec2 cn0_rPos = di.xy % ivec2(cn_dRes);
        uint cn0_rIdx = uint(cn0_rPos.x + cn0_rPos.y * cn_dRes);


        float rad = pixelToRadius(cn0_rPos, cn_dRes);


        // 4 probes
        vec4 colors[4];
        for (int jj = 0; jj < 4; ++jj)
        {
            ivec2 nextProbeIdx = cn1_baseProbeIdx + getOffsets(jj);

            vec4 mergedColor = vec4(0.f);
            // 4 rays
            for (int ii = 0; ii < 4; ++ii)
            {
                ivec2 texelPos = getTexelPos(nextProbeIdx, int(cn1_dRes), int(cn0_rIdx * 4 + ii));
                vec4 c = imageLoad(in_baseRc, ivec3(texelPos, currLv + 1));

                mergedColor += c;
            }
            mergedColor /= 4.f;

            colors[jj] = mergedColor;
        }


        vec4 color0 = mix(colors[0], colors[1], cn1_fract.x);
        vec4 color1 = mix(colors[2], colors[3], cn1_fract.x);
        vec4 color = mix(color0, color1, cn1_fract.y);

        vec4 c = mergeIntervals(baseColor, color);

        vec4 arrC = vec4(0.f);
        if (data.flags > 0)
        {
            vec2 mousePos = vec2(data.rc.mpx, data.rc.mpy);
            vec2 arrowuv = vec2(di.xy - mousePos);

            float arrow = sdf_arrow(arrowuv, 100.f, vec2(0.f, -1.f), 4.0, 2.0);
            arrC = mix(vec4(0.f), vec4(1.0), smoothstep(1.5, 0.0, arrow));
        }

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