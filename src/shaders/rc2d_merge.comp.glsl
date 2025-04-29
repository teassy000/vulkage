#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"
#include "rc_common2d.h"
#include "debug_gpu.h"


layout(push_constant) uniform block
{
    Rc2dMergeData data;
};

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0, RGBA8) uniform image2DArray in_rc; // rc data
layout(binding = 1, RGBA8) uniform image2DArray in_baseRc; // intermediate data, which is merged [current level + 1] + [next level + 1] into the current level
layout(binding = 2, RGBA8) uniform image2DArray merged_rc; // store the merged data


// merge intervals based on the alpha channel
// a = 0.0 means hitted
// a = 1.0 means not hitted
vec4 mergeIntervals(vec4 _near, vec4 _far)
{
    return vec4(_near.rgb + _near.a * _far.rgb, _near.a * _far.a);
}



ivec2 getTexelPos(ivec2 _pixIdx, int _dRes, ivec2 _off, int _rayIdx, ivec2 _baseIdx)
{
    ivec2 res = _pixIdx + _off * ivec2(_dRes);
    res.x += _rayIdx;

    ivec2 newIdx = res / _dRes;

    // shift to the next line
    if (newIdx.x != _baseIdx.x + _off.x)
    {
        res.x -= _dRes;
        res.y += 1;
        newIdx = res / _dRes;
    }
    // shift back to the start of probe
    if (newIdx.y != _baseIdx.y + _off.y)
    { 
        res.y -= _dRes;
    }

    return res;
}

ivec2 getTexelPos2(ivec2 _probeIdx, int _dRes, ivec2 _rayIdx)
{
    ivec2 res = _probeIdx * _dRes + _rayIdx;

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

    const uint currLv = data.lv;
    const uint factor = 1 << currLv;
    const uint nextLvFactor = factor * 2;
    const ivec2 screenSize = ivec2(data.rc.width, data.rc.height);

    const uint dRes = data.rc.c0_dRes * factor;
    ProbeSamp samp = getProbSamp(di.xy, dRes);

    const vec3 n0uv = vec3(vec2(di) / vec2(screenSize), float(currLv));
    vec4 baseColor = imageLoad(in_rc, ivec3(di, currLv));


    vec4 weights = getWeights(samp.ratio);

    const int nextdRes = int(data.rc.c0_dRes * nextLvFactor);
    ProbeSamp baseSamp = getProbSamp(di, nextdRes);

    float dStep = (2.f * PI) / float(dRes * dRes);
    float nextDStep = (2.f * PI) / float(nextdRes * nextdRes);

    ivec2 rayIdx = screenSize % baseSamp.baseIdx;
    ivec2 nextRayIdx = rayIdx * 2;

    vec4 mergedColor = vec4(0.f);

    // 4 probes
    for (int jj = 0; jj < 4; ++jj) 
    {
        ivec2 poff = getOffsets(jj);
        
        // 4 rays
        for (int ii = 0; ii < 4; ++ii)
        {
            ivec2 roff = getOffsets(ii);
            ivec2 texelPos = getTexelPos2(baseSamp.baseIdx + poff, nextdRes, nextRayIdx + roff);

            vec3 n1uv = vec3(vec2(texelPos) / vec2(screenSize), float(currLv + 1));
            vec4 color = imageLoad(in_baseRc, ivec3(texelPos, currLv + 1));

            mergedColor += mergeIntervals(baseColor, color) * weights[ii];
        }
    }


    imageStore(merged_rc, ivec3(di, currLv), mergedColor);
}