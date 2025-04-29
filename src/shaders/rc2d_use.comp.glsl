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
    Rc2dUseData data;
};

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform sampler2DArray in_rc;
layout(binding = 1) uniform sampler2DArray in_merged_rc;

layout(binding = 2, RGBA8) uniform image2D rt; // store the merged data

ivec2 getTexelPos(ivec2 _probeIdx, uint _rayIdx, uint _dRes)
{
    ivec2 dIdx = ivec2(_rayIdx % _dRes, _rayIdx / _dRes);
    dIdx.y = dIdx.y == _dRes ? 0 : dIdx.y;
    return _probeIdx * ivec2(_dRes) + dIdx;
}


void main()
{
    const ivec3 di = ivec3(gl_GlobalInvocationID.xyz);

    ivec2 resolution = ivec2(data.rc.width, data.rc.height);
    uint c0dRes = data.rc.c0_dRes;
    uint lv = min(data.rc.nCascades, data.lv);

    if (data.stage == 1)
    {


        vec3 uv = vec3(vec2(di.xy)/ vec2(resolution), float(lv));
        vec4 c = texture(in_rc, uv);
        imageStore(rt, ivec2(di.xy), vec4(c.rgb, 1.f));
        return;    
    }

    if (data.stage == 2)
    {
        vec3 uv = vec3(vec2(di.xy) / vec2(resolution), float(lv));
        vec4 c = texture(in_merged_rc, uv);
        imageStore(rt, ivec2(di.xy), vec4(c.rgb, 1.f));
        return;
    }


    vec3 color = vec3(0.f);
    vec2 uv = di.xy / vec2(resolution);
    vec4 sd = sdf(vec2(di.xy), vec2(resolution));
    if (sd.w < 0.5)
        color = sd.rgb;

    ProbeSamp samp = getProbSamp(di.xy, c0dRes);
    vec4 weights = getWeights(samp.ratio);

    float dStep = (2.f * PI) / float(c0dRes * c0dRes);
    // 4 rays per pixel

    vec3 mergedColor = vec3(0.f);
    for (int ii = 0; ii < 4; ++ii)
    {
        ivec2 offset = getOffsets(ii);

        vec2 pbCenter = (vec2(samp.baseIdx + offset) + vec2(.5f)) * float(c0dRes);
        vec2 rayDir = pbCenter - vec2(di.xy);
        if (compare(rayDir, vec2(0.f))) { 
            mergedColor = vec3(1.f, 0.f, 0.f);
            break;
        }

        rayDir = normalize(rayDir);

        // dir to rad
        float rad = atan(rayDir.y, rayDir.x); //[0, 2*PI]

        float ratio = rad / dStep;
        uint rIdx = uint(floor(ratio));
        ratio = fract(ratio);

        // get the texel position in the merged rc
        ivec2 t0 = getTexelPos(samp.baseIdx + offset, rIdx, c0dRes);
        vec2 uvn0 = vec2(t0) / vec2(resolution);

        ivec2 t1 = getTexelPos(samp.baseIdx + offset, rIdx + 1, c0dRes);
        vec2 uvn1 = vec2(t1) / vec2(resolution);

        vec4 col0 = texture(in_merged_rc, vec3(uvn0, 0.f));
        //col0.rgb *= col0.a;
        vec4 col1 = texture(in_merged_rc, vec3(uvn1, 0.f));
        //col1.rgb *= col1.a;

        vec3 mcol = mix(col0.rgb, col1.rgb, ratio);

        mergedColor += mcol * weights[ii];
    }

    color = mergedColor;

    vec4 arrC = vec4(0.f);
    if (data.arrow > 0)
    {
        vec2 mousePos = vec2(data.rc.mpx, data.rc.mpy);
        vec2 arrowuv = vec2(di.xy - mousePos);

        float arrow = sdf_arrow(arrowuv, 100.f, vec2(0.f, 1.f), 2.0, 4.0);
        arrC = mix(vec4(0.f), vec4(1.0), smoothstep(1.5, 0.0, arrow));
    }

    color.rgb = color.rgb * (1.0 - arrC.a) + arrC.rgb;

    imageStore(rt, ivec2(di.xy), vec4(color, 1.f));
}