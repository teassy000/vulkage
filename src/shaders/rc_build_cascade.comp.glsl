#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "rc_common.h"
#include "debug_gpu.h"
#include "ffx_brx_integrate.h"


layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(push_constant) uniform block
{
	RadianceCascadesConfig config;
};


layout(binding = 0) readonly uniform Transform
{
    TransformData trans;
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
}
in_cas[FFX_BRIXELIZER_MAX_CASCADES];


layout(binding = 4, set = 2) buffer readonly cascadeBrickMaps
{
    uint map[];
}in_cas_bricks[FFX_BRIXELIZER_MAX_CASCADES];

// ffx brixelizer required functions
// requred data:
// 1. the sdf atlas
// 2. the cascade info
// 3. the bricks aabb
// 4. the aabb trees
// 5. the cascade brick map array

FfxBrixelizerCascadeInfo GetCascadeInfo(FfxUInt32 _casId)
{
    return in_cascades_info[_casId];
}

FfxFloat32x3 LoadCascadeAABBTreesFloat3(FfxUInt32 _casId, FfxUInt32 _elemIdx) 
{
    return FfxFloat32x3(
              float(in_cas[_casId].aabb[_elemIdx + 0])
            , float(in_cas[_casId].aabb[_elemIdx + 1])
            , float(in_cas[_casId].aabb[_elemIdx + 2])
        );
}

FfxUInt32 LoadCascadeAABBTreesUInt(FfxUInt32 _casId, FfxUInt32 _elemIdx)
{
    return uint(in_cas[_casId].aabb[_elemIdx]);
}

FfxUInt32 LoadBricksAABB(FfxUInt32 _elemIdx) 
{
    return in_bricks_aabb[_elemIdx];
}

FfxFloat32 SampleSDFAtlas(FfxFloat32x3 _uvw)
{
    return texture(in_sdfAtlas, _uvw).x;
}

FfxUInt32 LoadCascadeBrickMapArrayUniform(FfxUInt32 _casId, FfxUInt32 _elemIdx)
{
    return uint(in_cas_bricks[_casId].map[_elemIdx]);
}

void main()
{
    const ivec2 pixIdx = ivec2(gl_GlobalInvocationID.xy);
    const uint lvLayer = gl_GlobalInvocationID.z;

    // the config for the radiance cascade.
    const uint prob_gridSideCount = config.probe_sideCount;
    const uint ray_gridSideCount = config.ray_gridSideCount;
    const float sceneRadius = config.sceneRadius;

    const ivec2 prob_idx = pixIdx / int(ray_gridSideCount);
    const ivec2 ray_idx = pixIdx % int(ray_gridSideCount);

    // the rc grid seg 
    // origin: the center of the probe in world space
    // direction: the direction of the seg
    // length: the probe radius
    const vec3 seg_origin = getCenterWorldPos(ivec3(prob_idx, lvLayer), sceneRadius, config.probeSideLen) + vec3(config.probeSideLen * 0.5f) + trans.cameraPos;
    const vec3 seg_dir = octDecode((vec2(ray_idx) + .5f) / float(ray_gridSideCount));
    const float seg_len = config.rayLength;

    // write the result to the atlas
    vec3 var = vec3(0.f);

    // ffx traverse
    FfxBrixelizerRayDesc ffx_ray;
    ffx_ray.start_cascade_id = 0;
    ffx_ray.end_cascade_id = 8;
    ffx_ray.t_min = 0.f;
    ffx_ray.t_max = 1000;
    ffx_ray.origin = seg_origin;
    ffx_ray.direction = seg_dir;

    FfxBrixelizerHitRaw ffx_hit;

    bool hit = FfxBrixelizerTraverseRaw(ffx_ray, ffx_hit);

    if (hit) {

        vec3 norm = FfxBrixelizerGetHitNormal(ffx_hit);
        var = norm;
    }
    else
    {
        var = vec3(1.f, 0.f, 0.f);
    }


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
    // vec3 ro = (seg_origin + sceneRadius) / (sceneRadius * 2.f);


    imageStore(out_octProbAtlas, idx_dir, vec4(var, 1.f));
}