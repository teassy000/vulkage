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

    const vec3 seg_origin = getCenterWorldPos(ivec3(prob_idx, lvLayer), rcRadius, config.probeSideLen) + vec3(config.probeSideLen * 0.5f) + centOffset + trans.cameraPos;

    vec2 uv = (vec2(ray_idx) + .5f) / float(ray_gridSideCount);
    // mapping uv to [-1, 1]
    uv = uv * 2.f - 1.f;
    const vec3 seg_dir = normalize(oct_to_float32x3(uv));
    const float seg_len = config.rayLength;

    // write the result to the atlas

    vec3 var = vec3(0.f);

    // ffx traverse
    FfxBrixelizerRayDesc ffx_ray;
    ffx_ray.start_cascade_id = config.brx_startCas + config.brx_offset;
    ffx_ray.end_cascade_id = config.brx_endCas + config.brx_offset;
    ffx_ray.t_min = config.brx_tmin;
    ffx_ray.t_max = seg_len;//config.brx_tmax;
    ffx_ray.origin = seg_origin;
    ffx_ray.direction = seg_dir;

    FfxBrixelizerHitRaw ffx_hit;

    bool hit = FfxBrixelizerTraverseRaw(ffx_ray, ffx_hit);

    if (hit) {
        vec3 norm = FfxBrixelizerGetHitNormal(ffx_hit);
        vec3 hit_pos = seg_origin + ffx_hit.t * seg_dir;

        // hit pos is in world space, now to uv space


        vec3 hit_uvw = (trans.proj * trans.view * vec4(hit_pos, 1.0)).xyz;

        float depth = texture(in_depth, hit_uvw.xy).x;
        if (depth > hit_uvw.z) {
            vec3 albedo = texture(in_albedo, hit_uvw.xy).xyz;
            vec3 emision = texture(in_emmision, hit_uvw.xy).xyz;

            var = albedo;
        }
    }

    // debug the probe hit_pos
    // var = (seg_origin - trans.cameraPos) / rcRadius + 0.5f;

    const uint layer_idx = lvLayer + config.layerOffset;
    uint pidx = uint(prob_idx.y * prob_gridSideCount + prob_idx.x) + lvLayer * prob_gridSideCount * prob_gridSideCount;

    uint mhash = hash(pidx);
    vec4 color = vec4(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255), 255) / 255.0;

    ivec2 dirOrderIdx = ivec2(ray_idx * int(prob_gridSideCount) + prob_idx);
    ivec3 idx_dir = ivec3(dirOrderIdx.xy, layer_idx);// seg dir idx first
    ivec3 idx = ivec3(pixIdx.xy, layer_idx); // probe idx first

    // debug: seg dir
    vec3 rd = (seg_dir.rgb + 1.f) * 0.5;
    // debug: seg origin
    // vec3 ro = (seg_origin + rcRadius) / (rcRadius * 2.f);

    ivec3 iuv = ivec3(0);
    switch(config.debug_type)
    {
        case 0:
        iuv = idx;
        break;
    case 1:
        iuv = idx_dir;
        break;
    case 2:
        iuv = idx;
        var = seg_dir;
        break;
    case 3:
        iuv = idx_dir;
        var = seg_dir;
        break;
    }


    imageStore(out_octProbAtlas, iuv, vec4(var, 1.f));
}