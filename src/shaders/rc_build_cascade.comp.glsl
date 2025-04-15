#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "rc_common.h"
#include "debug_gpu.h"

layout(push_constant) uniform block
{
    RadianceCascadesConfig config;
};

// predefined macros for ffx_brixelizer_trace_ops.h
#define FFX_GPU
#define FFX_GLSL
#define FFX_BRIXELIZER_TRAVERSAL_EPS config.brx_sdfEps
#define FFX_WAVE 1

#include "ffx_brixelizer_trace_ops.h"


layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) readonly uniform Transform
{
    RadianceCascadesTransform trans;
};

layout(binding = 1) uniform sampler2D in_albedo;
layout(binding = 2) uniform sampler2D in_normal;
layout(binding = 3) uniform sampler2D in_wPos;
layout(binding = 4) uniform sampler2D in_emmision;
layout(binding = 5) uniform sampler2D in_depth;
layout(binding = 6) uniform sampler2D in_skybox;

layout(binding = 7, RGBA8) uniform writeonly image2DArray out_octProbAtlas;

// ffx brixelizer data
layout(binding = 0, set = 2) uniform sampler3D in_sdfAtlas;

layout(binding = 1, set = 2) buffer readonly BrxCascadeInfos
{
    FfxBrixelizerCascadeInfo in_cascades_info[];
};

layout(binding = 2, set = 2) buffer readonly brickAABBTrees
{
    uint in_bricks_aabb[];
};

layout(binding = 3, set = 2) buffer readonly cascadeAABBTrees
{
    uint aabb[];
} in_cas_aabb_trees[FFX_BRIXELIZER_MAX_CASCADES];


layout(binding = 4, set = 2) buffer readonly cascadeBrickMaps
{
    uint map[];
} in_cas_brick_maps[FFX_BRIXELIZER_MAX_CASCADES];

// ffx brixelizer required functions
// FfxFloat32x3     LoadCascadeAABBTreesFloat3(FfxUInt32 cascadeID, FfxUInt32 elementIndex);
// FfxUInt32        LoadCascadeAABBTreesUInt(FfxUInt32 cascadeID, FfxUInt32 elementIndex);
// FfxUInt32        LoadBricksAABB(FfxUInt32 elementIndex);
// FfxBrixelizerCascadeInfo GetCascadeInfo(FfxUInt32 cascadeID);
// FfxFloat32       SampleSDFAtlas(FfxFloat32x3 uvw);
// FfxUInt32        LoadCascadeBrickMapArrayUniform(FfxUInt32 cascadeID, FfxUInt32 elementIndex);
// ================================================================================================
// required data:
// 1. the sdf atlas
// 2. the cascade info
// 3. the bricks aabb
// 4. the aabb trees array
// 5. the cascade brick map array
FfxBrixelizerCascadeInfo GetCascadeInfo(FfxUInt32 _casId)
{
    return in_cascades_info[_casId];
}

FfxFloat32x3 LoadCascadeAABBTreesFloat3(FfxUInt32 _casId, FfxUInt32 _elemIdx) 
{
    return FfxFloat32x3(
              uintBitsToFloat(in_cas_aabb_trees[_casId].aabb[_elemIdx + 0])
            , uintBitsToFloat(in_cas_aabb_trees[_casId].aabb[_elemIdx + 1])
            , uintBitsToFloat(in_cas_aabb_trees[_casId].aabb[_elemIdx + 2])
        );
}

FfxUInt32 LoadCascadeAABBTreesUInt(FfxUInt32 _casId, FfxUInt32 _elemIdx)
{
    return uint(in_cas_aabb_trees[_casId].aabb[_elemIdx]);
}

FfxUInt32 LoadBricksAABB(FfxUInt32 _elemIdx) 
{
    return in_bricks_aabb[_elemIdx];
}

FfxFloat32 SampleSDFAtlas(FfxFloat32x3 _uvw)
{
    return textureLod(in_sdfAtlas, _uvw, 0.f).x;
}

FfxUInt32 LoadCascadeBrickMapArrayUniform(FfxUInt32 _casId, FfxUInt32 _elemIdx)
{
    return uint(in_cas_brick_maps[_casId].map[_elemIdx]);
}

bool inScreenSpaceRange(vec3 _uvw, float _depth)
{
    bool res = true;
    res = res && (_uvw.x >= 0.f && _uvw.x <= 1.f);
    res = res && (_uvw.y >= 0.f && _uvw.y <= 1.f);
    res = res && (_uvw.z >= _depth && _uvw.z <= 1.f);

    return res;
}

void main()
{
    const ivec2 pixIdx = ivec2(gl_GlobalInvocationID.xy);
    const uint lvLayer = gl_GlobalInvocationID.z;

    // the config for the radiance cascade.
    const uint prob_gridSideCount = config.probe_sideCount;
    const uint ray_gridSideCount = config.ray_gridSideCount;
    const float rcRadius = config.radius;

    const ivec2 prob_idx = pixIdx / int(ray_gridSideCount);
    const ivec2 ray_idx = pixIdx % int(ray_gridSideCount);

    // the rc grid seg 
    // origin: the center of the probe in world space
    // direction: the direction of the seg
    // length: the probe radius
    const vec3 centOffset = vec3(config.cx, config.cy, config.cz);

    const vec3 seg_origin = getCenterWorldPos(ivec3(prob_idx, lvLayer), rcRadius, config.probeSideLen) + centOffset + trans.cameraPos;

    vec2 uv = (vec2(ray_idx) + .5f) / float(ray_gridSideCount);
    
    uv = uv * 2.f - 1.f; // to [-1.f, 1.f]
    const vec3 seg_dir = normalize(oct_to_float32x3(uv));
    const float seg_len = config.rayLength;

    // ffx traverse
    FfxBrixelizerRayDesc ffx_ray;
    ffx_ray.start_cascade_id = config.brx_startCas + config.brx_offset;
    ffx_ray.end_cascade_id = config.brx_endCas + config.brx_offset;
    ffx_ray.t_min = config.brx_tmin;
    ffx_ray.t_max = seg_len;//config.brx_tmax
    ffx_ray.origin = seg_origin;
    ffx_ray.direction = seg_dir;

    FfxBrixelizerHitRaw ffx_hit;

    bool hit = FfxBrixelizerTraverseRaw(ffx_ray, ffx_hit);

    vec3 albedo = vec3(0.f);
    vec3 normal = vec3(0.f);
    vec3 wpos = vec3(0.f);
    vec3 emision = vec3(0.f);
    float hit_distance = ffx_ray.t_max;
    vec3 hit_normal = vec3(0.f);

    if (hit) {
        vec3 norm = FfxBrixelizerGetHitNormal(ffx_hit);
        vec3 hit_pos = seg_origin + ffx_hit.t * seg_dir;

        // hit pos is in world space, now to uv space
        vec4 hit_ppos = (trans.proj * trans.view * vec4(hit_pos, 1.0)).xyzw;
        
        hit_ppos.xyz /= hit_ppos.w;
        hit_ppos.y = -hit_ppos.y; // flip y


        vec3 hit_uvw = vec3(vec2(hit_ppos.xy * 0.5f + 0.5f), hit_ppos.z); // map to [0, 1]

        float depth = texture(in_depth, hit_uvw.xy).x;
        if (inScreenSpaceRange(hit_uvw, depth)) {
            albedo = texture(in_albedo, hit_uvw.xy).xyz;
            normal = texture(in_normal, hit_uvw.xy).xyz;
            wpos = texture(in_wPos, hit_uvw.xy).xyz;
            emision = texture(in_emmision, hit_uvw.xy).xyz;
        }
        hit_distance = ffx_hit.t;
        hit_normal = norm;
    }

    const uint layer_idx = lvLayer + config.layerOffset;

    ivec3 iuv = ivec3(0);
    switch(config.debug_idx_type)
    {
        case 0:
            iuv = ivec3(pixIdx.xy, layer_idx); // probe idx first;
            break;
        case 1:
            ivec2 dirOrderIdx = ivec2(ray_idx * int(prob_gridSideCount) + prob_idx);
            iuv = ivec3(dirOrderIdx.xy, layer_idx);// seg dir idx first
            break;
    }

    // write the var to the radiance cascade atlas
    vec3 var = vec3(0.f);
    switch (config.debug_color_type) {
        case 0:
            var = albedo;
            break;
        case 1:
            var = normal;
            break;
        case 2:
            var = normalize(wpos);
            break;
        case 3:
            var = emision;
            break;
        case 4:
            var = vec3(1.f - (hit_distance / ffx_ray.t_max));
            break;
        case 5:
            var = hit_normal;
            break;
        case 6:
            var = seg_dir * .5f + .5f;
            break;
        case 7:
            var = vec3(seg_origin) / rcRadius;
            break;
        case 8:
            uint pidx = uint(prob_idx.y * prob_gridSideCount + prob_idx.x) + lvLayer * prob_gridSideCount * prob_gridSideCount;
            uint mhash = hash(pidx);
            var = vec3(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255)) / 255.0;
            break;
    }
    float hitvar = hit ? 1.f : 0.f;
    imageStore(out_octProbAtlas, iuv, vec4(var, hitvar));
}