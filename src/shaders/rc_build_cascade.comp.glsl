#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "rc_common.h"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

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

layout(binding = 6) readonly buffer OctTreeNodeCount
{
    uint in_octTreeNodeCount;
};

layout(binding = 7) readonly buffer OctTree
{
    OctTreeNode in_octTree [];
};


layout(binding = 8, RGBA8) uniform imageBuffer in_voxAlbedo;
layout(binding = 9, RGBA8) uniform writeonly image2DArray octProbAtlas;

struct Ray
{
    vec3 origin;
    vec3 dir;
    float len;
};

struct AABB
{
    vec3 min;
    vec3 max;
};

void getAABB(vec3 _certer, float _hSideLen, out AABB _aabb)
{
    _aabb.max = _certer + vec3(_hSideLen);
    _aabb.min = _certer - vec3(_hSideLen);
}

// https://tavianator.com/2011/ray_box.html
bool intersect(const Ray _ray, const AABB _aabb)
{
    vec3 invDir = 1.0 / _ray.dir;
    vec3 t0s = (_aabb.min - _ray.origin) * invDir;
    vec3 t1s = (_aabb.max - _ray.origin) * invDir;

    vec3 tsmaller = min(t0s, t1s);
    vec3 tbigger = max(t0s, t1s);

    float tmin = max(max(tsmaller.x, tsmaller.y), tsmaller.z);
    float tmax = min(min(tbigger.x, tbigger.y), tbigger.z);

    return tmax >= tmin && _ray.len >= tmin;
}

void main()
{
    // the config for the radiance cascade.
    const uint prob_diameter = config.probe_sideCount;
    const uint ray_diameter = config.ray_gridSideCount;

    const uint prob_per_layer = prob_diameter * prob_diameter;
    const uint ray_count = ray_diameter * ray_diameter;
    const uint prob_ray_idx = gl_GlobalInvocationID.x % ray_count;
    const float scene_side_len = config.ot_sceneSideLen;

    // Calculate the rayDir index
    uint prob_idx = gl_GlobalInvocationID.x / ray_count;
    uint prob_layer_idx = prob_idx % prob_per_layer;
    uint layer_idx = prob_idx / prob_per_layer + config.layerOffset;

    ivec2 sub_prob_coord = ivec2(prob_layer_idx % prob_diameter, prob_layer_idx / prob_diameter);
    ivec2 sub_ray_coord = ivec2(prob_ray_idx % ray_diameter, prob_ray_idx / ray_diameter);
    vec2 prob_2dcoord = sub_prob_coord * ray_diameter + sub_ray_coord;

    // the rc grid ray 
    // origin: the center of the probe in world space
    // direction: the direction of the ray
    // length: the probe radius
    const ivec3 probe_pos = ivec3(sub_prob_coord.xy, prob_idx / prob_per_layer);
    const vec3 ray_origin = vec3(probe_pos) * config.probeSideLen + config.probeSideLen * .5f;
    const vec3 ray_dir = octDecode((sub_ray_coord + .5f) / ray_diameter);
    const float ray_len = config.rayLength;// use the longest diagnal length to make sure it covers the entire probe

    const Ray ray = Ray(ray_origin, ray_dir, ray_len);
    
    
    AABB aabb;
    getAABB(vec3(0), scene_side_len * .5f, aabb);
    OctTreeNode root = in_octTree[in_octTreeNodeCount - 1];
    uint treeLevel = 0;

    // traversal the octree to check the intersection
    // intersect(ray, aabb);


    imageStore(octProbAtlas, ivec3(prob_2dcoord.xy, layer_idx), vec4(ray_dir, 1));
}