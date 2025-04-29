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

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

 
layout(binding = 0, RGBA8) uniform image2DArray out_rc; // store the merged data

struct Ray
{ 
    vec2 origin;
    vec2 dir;
};

// trace ray in the pixel space
vec4 traceRay(Ray _ray, float _tmin, float _tmax, vec2 _res)
{
    float t = _tmin;
    for (uint ii = 0; ii < 10; ++ii)
    {
        vec4 sd = sdf(_ray.origin + _ray.dir * t, _res);
        if (sd.w < .5f) 
            return vec4(sd.rgb, 0.f);
        
        t += sd.w;
        if (t > _tmax) 
            break;
    }

    return vec4(0.f, 0.f, 0.f, 1.f);
}

void main()
{
    const ivec3 di = ivec3(gl_GlobalInvocationID.xyz);
    const ivec2 res = ivec2(data.width, data.height);

    uint factor = 1 << di.z;
    uint rayPxlCnt = factor * data.c0_dRes;
    vec2 probeCnt = vec2(data.width, data.height) / float(rayPxlCnt);
    vec2 probeIdx = vec2(di.xy) / float(rayPxlCnt);
    vec2 subUV = fract(probeIdx);

    // origin should be the center of the probe;
    vec2 probeTexel = (floor(probeIdx) + vec2(.5f)) * float(rayPxlCnt);

    float radStep = 2.f * PI / float(rayPxlCnt * rayPxlCnt);
    ivec2 subPixId = ivec2(di.x % rayPxlCnt, di.y % rayPxlCnt);
    uint subPixIdx = subPixId.x + subPixId.y * rayPxlCnt;
    float rad = float(subPixIdx + .5f) * radStep - PI;

    Ray ray;
    ray.origin = vec2(probeTexel);
    ray.dir = vec2(cos(rad), sin(rad));

    float c0Len = length(res) * 4.f / ((1 << 2 * data.nCascades) - 1.f);
    float tmin = di.z == 0 ? 0.f : c0Len * float(1 << 2 * (di.z - 1));
    float tmax = c0Len * float(1 << 2 * di.z);

    vec4 color = vec4(0.f);
    color = traceRay(ray, tmin, tmax, vec2(res));

    imageStore(out_rc, ivec3(di.xyz), color);
}