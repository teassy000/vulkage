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
layout(binding = 6) uniform sampler2D in_skybox;

layout(binding = 7) readonly buffer OctTreeNodeCount
{
    uint in_octTreeNodeCount;
};

layout(binding = 8) readonly buffer OctTree
{
    OctTreeNode in_octTree [];
};


layout(binding = 9, RGBA8) uniform imageBuffer in_voxAlbedo;
layout(binding = 10, RGBA8) uniform writeonly image2DArray octProbAtlas;

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

struct OT_UnfoldedNode
{
    vec3 center;
    float sideLen;
    AABB aabb;
};

void getAABB(vec3 _certer, float _hSideLen, out AABB _aabb)
{
    _aabb.max = _certer + vec3(_hSideLen);
    _aabb.min = _certer - vec3(_hSideLen);
}

void unfoldOctTreeChildNode(OT_UnfoldedNode _p, uint _cIdx, inout OT_UnfoldedNode _c)
{
    ivec3 cPos = ivec3(_cIdx & 1, (_cIdx >> 1) & 1, (_cIdx >> 2) & 1);

    _c.sideLen = _p.sideLen * .5f;
    _c.center = _p.center + _c.sideLen * .5f * (vec3(cPos) * 2.f - 1.f);

    getAABB(_c.center, _c.sideLen * .5f, _c.aabb);
}

// a modified version of line-ray intersection
// with a ray length check, aka the ray is a segment
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

// Sort the 'dist' array in ascending order while swapping the corresponding child indices.
void insertionSort8(const uint _count, inout float _dist[8], inout OT_UnfoldedNode _childs[8], inout uint _cidx[8])
{
    for (uint ii = 1; ii < _count; ii++)
    {
        float keyDist = _dist[ii];
        OT_UnfoldedNode keyChild = _childs[ii];
        uint keyCidx = _cidx[ii];

        int jj = int(ii) - 1;
        // Shift elements while dist[jj] > keyDist
        while (jj >= 0 && _dist[jj] > keyDist)
        {
            _dist[jj + 1] = _dist[jj];
            _childs[jj + 1] = _childs[jj];
            _cidx[jj + 1] = _cidx[jj];
            jj--;
        }

        _dist[jj + 1] = keyDist;
        _childs[jj + 1] = keyChild;
        _cidx[jj + 1] = keyCidx;
    }
}

void sortChilds(Ray _ray, uint _count, inout OT_UnfoldedNode _childs[8], inout uint _cIdx[8])
{
    const vec3 ray_dir = _ray.dir;
    const vec3 ray_origin = _ray.origin;

    float dist[8];
    for (uint ii = 0; ii < _count; ++ii)
        dist[ii] = dot(_childs[ii].center - ray_origin, ray_dir);


    // ascending sort
    insertionSort8(_count, dist, _childs, _cIdx);
}

void main()
{
    // the config for the radiance cascade.
    const uint prob_gridSideCount = config.probe_sideCount;
    const uint ray_gridSideCount = config.ray_gridSideCount;

    const uint prob_cnt_per_layer = prob_gridSideCount * prob_gridSideCount;
    const uint ray_count = ray_gridSideCount * ray_gridSideCount;
    const uint ray_idx = gl_GlobalInvocationID.x % ray_count;
    const float sceneRadius = config.ot_sceneRadius;

    // Calculate the rayDir index
    uint prob_idx = gl_GlobalInvocationID.x / ray_count;
    uint prob_layer_idx = prob_idx % prob_cnt_per_layer;
    uint layer_idx = prob_idx / prob_cnt_per_layer + config.layerOffset;

    ivec2 sub_prob_coord = ivec2(prob_layer_idx % prob_gridSideCount, prob_layer_idx / prob_gridSideCount);
    ivec2 sub_ray_coord = ivec2(ray_idx % ray_gridSideCount, ray_idx / ray_gridSideCount);
    ivec2 prob_2dcoord = sub_prob_coord * ivec2(ray_gridSideCount) + sub_ray_coord; // position first, each grid is a probe
    ivec2 prob_2dcorrd_dir = sub_ray_coord * ivec2(prob_gridSideCount) + sub_prob_coord; // direction first, each grid is a ray


    const vec3 scene_origin_offset = vec3(-sceneRadius);

    // the rc grid ray 
    // origin: the center of the probe in world space
    // direction: the direction of the ray
    // length: the probe radius
    const ivec3 probe_pos = ivec3(sub_prob_coord.xy, prob_idx / prob_cnt_per_layer);
    const vec3 ray_origin = vec3(probe_pos) * config.probeSideLen + config.probeSideLen * .5f + scene_origin_offset;
    const vec3 ray_dir = octDecode((sub_ray_coord + .5f) / ray_gridSideCount);
    const float ray_len = config.rayLength;// use the longest diagnal length to make sure it covers the entire probe

    const Ray ray = Ray(ray_origin, ray_dir, ray_len);
    
    
    OctTreeNode node = in_octTree[in_octTreeNodeCount - 1];
    uint treeLevel = 0;

    OT_UnfoldedNode uNode;
    uNode.center = vec3(0.f);
    uNode.sideLen = sceneRadius * 2.f;
    getAABB(uNode.center, uNode.sideLen * .5f, uNode.aabb);

    // traversal the octree to check the intersection
    uint voxIdx = INVALID_OCT_IDX;

    while (true)
    {
        // unfold childs 
        OT_UnfoldedNode childs[8];
        uint cIdx[8];
        uint validCount = 0;
        for (uint ii = 0; ii < 8; ii++)
        {
            if (node.childs[ii] == INVALID_OCT_IDX)
                continue;

            unfoldOctTreeChildNode(uNode, ii, childs[validCount]);
            cIdx[validCount] = ii;
            validCount++;
        }

        // sort childs
        sortChilds(ray, validCount, childs, cIdx);
        
        uint nextIdx = INVALID_OCT_IDX;
        // intersect with the child nodes
        for (uint ii = 0; ii < validCount; ii++)
        {
            if (intersect(ray, childs[ii].aabb))
            {
                nextIdx = cIdx[ii];
                uNode = childs[ii];
                node = in_octTree[node.childs[nextIdx]];
                break;
            }
        }

        if(nextIdx == INVALID_OCT_IDX)
        {
            break;
        }
        else if(nextIdx < validCount)
        {
            if (node.lv == 0)
            {
                voxIdx = node.childs[cIdx[nextIdx]];
                break;
            }
        }
    }

    // write the result to the atlas
    vec3 var = vec3(0.f);
    if (voxIdx != INVALID_VOX_ID)
    {
        var = imageLoad(in_voxAlbedo, int(voxIdx)).rgb;
    }


    imageStore(octProbAtlas, ivec3(prob_2dcorrd_dir.xy, layer_idx), vec4(var, 1.f));
}