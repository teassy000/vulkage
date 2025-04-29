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
    Rc2dData data;
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

    vec3 color = vec3(0.f);
    vec2 uv = di.xy / vec2(data.width, data.height);
    vec4 sd = sdf(vec2(di.xy), vec2(data.width, data.height));
    if (sd.w < 0.5)
        color = sd.rgb;

    ProbeSamp samp = getProbSamp(di.xy, data.c0_dRes);
    vec4 weights = getWeights(samp.ratio);

    float dStep = (2.f * PI) / float(data.c0_dRes * data.c0_dRes);
    // 4 rays per pixel

    vec3 mergedColor = vec3(0.f);
    for (int ii = 0; ii < 4; ++ii)
    {
        ivec2 offset = getOffsets(ii);

        vec2 pbCenter = (vec2(samp.baseIdx + offset) + vec2(.5f)) * float(data.c0_dRes);
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
        ivec2 t0 = getTexelPos(samp.baseIdx + offset, rIdx, data.c0_dRes);
        vec2 uvn0 = vec2(t0) / vec2(data.width, data.height);

        ivec2 t1 = getTexelPos(samp.baseIdx + offset, rIdx + 1, data.c0_dRes);
        vec2 uvn1 = vec2(t1) / vec2(data.width, data.height);

        vec4 col0 = texture(in_merged_rc, vec3(uvn0, 0.f));
        //col0.rgb *= col0.a;
        vec4 col1 = texture(in_merged_rc, vec3(uvn1, 0.f));
        //col1.rgb *= col1.a;

        vec3 mcol = mix(col0.rgb, col1.rgb, ratio);

        mergedColor += mcol * weights[ii];
    }

    color = mergedColor;

    vec4 c = texture(in_rc, vec3(uv, 2.f));

    color = c.rgb;
    imageStore(rt, ivec2(di.xy), vec4(color, 1.f));
}