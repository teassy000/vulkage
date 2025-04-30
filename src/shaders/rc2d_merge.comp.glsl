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

        const uint cn_dRes = data.rc.c0_dRes * factor;
        const uint cn1_dRes = data.rc.c0_dRes * nextLvFactor;

        ProbeSamp baseSamp = getProbSamp(di.xy, cn_dRes);

        const vec3 n0uv = vec3(vec2(di) / vec2(screenSize), float(currLv));
        vec4 baseColor = imageLoad(in_rc, ivec3(di, currLv));

        vec4 weights = getWeights(baseSamp.ratio);


        ProbeSamp nxtSamp = getNextLvProbeSamp(baseSamp);

        ivec2 rayPos = di.xy % ivec2(cn_dRes);
        uint rayIdx = uint(rayPos.x + rayPos.y * cn_dRes);

        vec4 mergedColor = vec4(0.f);


        // 4 probes
        //for (int jj = 0; jj < 4; ++jj)
        {
            // 4 rays per pixel
            for (int ii = 0; ii < 4; ++ii)
            {
                ivec2 texelPos = getTexelPos(nxtSamp.baseIdx, int(cn1_dRes), int(rayIdx * 4 + ii));
                vec4 c = imageLoad(in_baseRc, ivec3(texelPos, currLv + 1));

                mergedColor += mergeIntervals(baseColor, c) * weights[ii];
            }
        }


        vec4 arrC = vec4(0.f);
        if (data.arrow > 0)
        {
            vec2 mousePos = vec2(data.rc.mpx, data.rc.mpy);
            vec2 arrowuv = vec2(di.xy - mousePos);

            float arrow = sdf_arrow(arrowuv, 100.f, vec2(0.f, -1.f), 4.0, 2.0);
            arrC = mix(vec4(0.f), vec4(1.0), smoothstep(1.5, 0.0, arrow));
        }

        imageStore(merged_rc, ivec3(di, currLv), mergedColor);
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