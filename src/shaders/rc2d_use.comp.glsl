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
layout(binding = 2) uniform sampler2DArray in_merged_probe;

layout(binding = 3, RGBA8) uniform image2D rt; // store the merged data

ivec2 getTexelPos(ivec2 _probeIdx, uint _rayIdx, uint _dRes)
{
    ivec2 dIdx = ivec2(_rayIdx % _dRes, _rayIdx / _dRes);
    dIdx.y = dIdx.y == _dRes ? 0 : dIdx.y;
    return _probeIdx * ivec2(_dRes) + dIdx;
}


vec4 bilinearInterpolate(sampler2DArray tex, vec2 _uv)
{
    vec2 texSz = textureSize(tex, 0).xy;
    vec2 lowResTexCoord = _uv * texSz;

    vec2 texelIdx = floor(lowResTexCoord);
    vec2 texelFract = fract(lowResTexCoord);

    vec2 offsets = vec2(0.f, 1.f);
    vec2 baseUV = texelIdx / texSz;
    vec2 step = 0.8f / texSz;


    vec4 color00 = texture(tex, vec3(baseUV + offsets.xx * step, 0.f));
    vec4 color10 = texture(tex, vec3(baseUV + offsets.yx * step, 0.f));
    vec4 color01 = texture(tex, vec3(baseUV + offsets.xy * step, 0.f));
    vec4 color11 = texture(tex, vec3(baseUV + offsets.yy * step, 0.f));

    vec4 color0 = mix(color00, color10, texelFract.x);
    vec4 color1 = mix(color01, color11, texelFract.x);
    vec4 color = mix(color0, color1, texelFract.y);

    return color;
}

void main()
{
    const ivec3 di = ivec3(gl_GlobalInvocationID.xyz);

    ivec2 resolution = ivec2(data.rc.width, data.rc.height);
    uint c0dRes = data.rc.c0_dRes;
    uint lv = min(data.rc.nCascades, data.lv);


    vec3 color = vec3(0.f);
    if (data.stage == 0)
    {
        vec2 uv = di.xy / vec2(resolution);

        ProbeSamp samp = getProbSamp(di.xy, c0dRes);
        vec4 weights = getWeights(samp.ratio);

        float dStep = (2.f * PI) / float(c0dRes * c0dRes);

        vec3 mergedColor = bilinearInterpolate(in_merged_probe, uv).rgb;

        color = mergedColor;
    }
    else if (data.stage == 1)
    {
        vec3 uv = vec3(vec2(di.xy)/ vec2(resolution), float(lv));
        vec4 c = texture(in_rc, uv);
        color = c.rgb; 
    }
    else if (data.stage == 2)
    {
        vec3 uv = vec3(vec2(di.xy) / vec2(resolution), float(lv));
        vec4 c = texture(in_merged_rc, uv);
        color = c.rgb;
    }
    else if (data.stage == 3)
    {
        vec3 uv = vec3(vec2(di.xy) / vec2(resolution), float(lv));
        vec4 c = texture(in_merged_probe, uv);
        color = c.rgb;
    }


    bool arrow = (data.flags & (1 << 0) ) > 0;
    bool border =( data.flags & (1 << 1) ) > 0;
    if (arrow)
    {
        vec4 arrC = vec4(0.f);

        vec2 mousePos = vec2(data.rc.mpx, data.rc.mpy);
        vec2 arrowuv = vec2(di.xy - mousePos);

        float arrow = sdf_arrow(arrowuv, 100.f, vec2(0.f, -1.f), 4.0, 2.0);
        arrC = mix(vec4(0.f), vec4(1.0), smoothstep(1.5, 0.0, arrow));
        color.rgb = color.rgb * (1.0 - arrC.a) + arrC.rgb;
    }

    if (border)
    {
        uint factor = 1 << lv;
        uint cn_dRes = data.rc.c0_dRes * factor;

        if (di.x % cn_dRes == 0 || (di.x + 1) % cn_dRes == 0
            || di.y % cn_dRes == 0 || (di.y + 1) % cn_dRes == 0)
            color = vec3(0.f, 0.f, 0.f);
    }

    imageStore(rt, ivec2(di.xy), vec4(color, 1.f));
}