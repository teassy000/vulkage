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

layout(binding = 7, RGBA8) uniform writeonly image2DArray octProbAtlas;
layout(binding = 8, R8) uniform writeonly image3D sdfAtlas;

struct Segment
{
    vec3 origin;
    vec3 dir;
    float len;
};

struct Plane
{
    vec3 normal;
    float dist;
};

struct AABB
{
    vec3 min;
    vec3 max;
};

struct OT_UnfoldedNode
{
    uint id;
    vec3 center;
    float sideLen;
    AABB aabb;
};

void getAABB(vec3 _certer, float _hSideLen, out AABB _aabb)
{
    _aabb.max = _certer + vec3(_hSideLen);
    _aabb.min = _certer - vec3(_hSideLen);
}

void unfoldOctTreeChildNode(OT_UnfoldedNode _p, uint _id, uint _cIdx, inout OT_UnfoldedNode _c)
{
    ivec3 cPos = ivec3(_cIdx & 1, (_cIdx >> 1) & 1, (_cIdx >> 2) & 1);

    _c.sideLen = _p.sideLen * .5f;
    _c.center = _p.center + _c.sideLen * .5f * (vec3(cPos) * 2.f - 1.f);
    _c.id = _id;

    getAABB(_c.center, _c.sideLen * .5f, _c.aabb);
}

// a modified version of line-seg intersection
// with a seg length check, aka the seg is a segment
// https://tavianator.com/2011/ray_box.html
bool intersect(const Segment _ray, const AABB _aabb)
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

// based on method of `ClosestChild` in following slides
// https://bertolami.com/files/octrees.pdf
// the method is trying to determine which quadrant the ray origin would be in the coordinate of the node
// it does not care about the intersection
//
// with modification: 
// 1. consider when ori_point could be 0.f
uint determine_nearest_child(const Segment _seg, const OT_UnfoldedNode _pNode)
{
    vec3 ori_point = _seg.origin - _pNode.center;

    if(compare(ori_point, vec3(0.f)) ) {
        ori_point = _seg.dir;
    } 

    uint x_test = ori_point.x >= 0.f ? 1u : 0u;
    uint y_test = ori_point.y >= 0.f ? 1u : 0u;
    uint z_test = ori_point.z >= 0.f ? 1u : 0u;

    uint ret = x_test | y_test << 1u | (z_test << 2u);

    return ret;
}

bool seg_intersection_plane(const Segment _seg, const Plane _p, inout float _dist)
{
    // the projection of the seg direction on the plane normal
    float ts = dot(_seg.dir, _p.normal);

    if (!compare(ts, 0.f))
    {
        // the projection of the seg origin on the plane normal
        float ns = -dot(_seg.origin, _p.normal) - _p.dist;

        float t = ns / ts;
        _dist = t;
        return t >= 0.f && t <= _seg.len;
    }

    return false;
}

bool traceChilds(const Segment _seg, const OT_UnfoldedNode _node, const OT_UnfoldedNode _childs[8], out uint _cidx)
{
    if (_node.id == INVALID_OCT_IDX)
        return false;

    uint cidx = determine_nearest_child(_seg, _node);

    // the seg is outside the node
    // the seg is not parallel to the plane
    // the seg is intersect with the plane
    Plane planes[3];
    planes[0] = Plane(vec3(1.f, 0.f, 0.f), _node.center.x); // yz
    planes[1] = Plane(vec3(0.f, 1.f, 0.f), _node.center.y); // xz
    planes[2] = Plane(vec3(0.f, 0.f, 1.f), _node.center.z); // xy

    bool planeIntersect[3];
    float planeDist[3];
    for (uint ii = 0; ii < 3; ii++)
    {
        planeIntersect[ii] = seg_intersection_plane(_seg, planes[ii], planeDist[ii]);
    }

    for (uint ii = 0; ii < 4; ii++)
    {
        OT_UnfoldedNode ch = _childs[cidx];

        if (ch.id == INVALID_OCT_IDX)
            continue;

        if (intersect(_seg, ch.aabb)) {
            _cidx = cidx;
            return true;
        }
        else if (planeIntersect[0] && planeDist[0] < planeDist[1] && planeDist[0] < planeDist[2]) {
            cidx ^= 1u;
            planeIntersect[0] = false;
            planeDist[0] = FLT_MAX;
        }
        else if (planeIntersect[1] && planeDist[1] < planeDist[0] && planeDist[1] < planeDist[2]) {
            cidx ^= 4u;
            planeIntersect[1] = false;
            planeDist[1] = FLT_MAX;
        }
        else if (planeIntersect[2] && planeDist[2] < planeDist[0] && planeDist[2] < planeDist[1]) {
            cidx ^= 2u;
            planeIntersect[2] = false;
            planeDist[2] = FLT_MAX;
        }
        else {
            break;
        }
    }

    return false;
}

// ffx brixelizer required functions
// requred data:
// 1. the cascade info
// 2. the aabb trees
// 3. the bricks aabb
// 4. the sdf atlas
// 5. the cascade brick map array

FfxBrixelizerCascadeInfo GetCascadeInfo(FfxUInt32 cascadeID)
{
    FfxBrixelizerCascadeInfo info;
    return info;
}

FfxFloat32x3 LoadCascadeAABBTreesFloat3(FfxUInt32 cascadeID, FfxUInt32 elementIndex) 
{
    return vec3(0.f);
}
FfxUInt32 LoadCascadeAABBTreesUInt(FfxUInt32 cascadeID, FfxUInt32 elementIndex)
{
    return 0;
}
FfxUInt32 LoadBricksAABB(FfxUInt32 elementIndex) 
{
    return 0;
}
FfxFloat32 SampleSDFAtlas(FfxFloat32x3 uvw)
{
    return 0.f;
}
FfxUInt32 LoadCascadeBrickMapArrayUniform(FfxUInt32 cascadeID, FfxUInt32 elementIndex)
{ 
    return 0;
}

void main()
{
    const ivec2 pixIdx = ivec2(gl_GlobalInvocationID.xy);
    const uint lvLayer = gl_GlobalInvocationID.z;

    // the config for the radiance cascade.
    const uint prob_gridSideCount = config.probe_sideCount;
    const uint ray_gridSideCount = config.ray_gridSideCount;
    const float sceneRadius = config.ot_sceneRadius;

    const ivec2 prob_idx = pixIdx / int(ray_gridSideCount);
    const ivec2 ray_idx = pixIdx % int(ray_gridSideCount);

    // the rc grid seg 
    // origin: the center of the probe in world space
    // direction: the direction of the seg
    // length: the probe radius
    const vec3 seg_origin = getCenterWorldPos(ivec3(prob_idx, lvLayer), sceneRadius, config.probeSideLen) + vec3(config.probeSideLen * 0.5f);
    const vec3 seg_dir = -octDecode((vec2(ray_idx) + .5f) / float(ray_gridSideCount));
    const float seg_len = config.rayLength;

    const Segment seg = Segment(seg_origin, seg_dir, seg_len);
    


    // write the result to the atlas
    vec3 var = vec3(0.f);

    var = vec3(1.f, 0.f, 0.f);

    //var = vec3(voxIdx & 255u, (voxIdx >> 8) & 255u, (voxIdx >> 16) & 255u) / 255.f;



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


    imageStore(octProbAtlas, idx_dir, vec4(var, 1.f));
}