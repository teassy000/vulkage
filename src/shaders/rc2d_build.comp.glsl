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
    uint cn_dRes = factor * data.c0_dRes;


    ProbeSamp samp = getProbSamp(di.xy, cn_dRes);

    // origin should be the center of the probe;
    vec2 probeCenter = (vec2(samp.baseIdx) + vec2(.5f)) * float(cn_dRes);

    float rad = pixelToRadius(di.xy, cn_dRes);
    
    Ray ray;
    ray.origin = vec2(probeCenter);
    ray.dir = vec2(cos(rad), sin(rad));

    float c0Len = data.c0_rLen;
    float tmin = di.z == 0 ? 0.f : c0Len * float(1 << 2 * (di.z - 1));
    float tmax = c0Len * float(1 << 2 * di.z);

    vec4 color = vec4(0.f);
    color = traceRay(ray, tmin, tmax, vec2(res));

    //color = vec4(ray.dir * .5f + .5f, 0.f, 1.f); ;
    imageStore(out_rc, ivec3(di.xyz), color);
}