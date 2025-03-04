#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "rc_common.h"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(push_constant) uniform blocks
{
    OctTreeProcessConfig conf;
};

layout(binding = 0) readonly buffer VoxMap
{
	uint voxMap[];
};

layout(binding = 1) buffer VoxMediumMap
{
    uint voxMediumMap[];
};

layout(binding = 2) buffer OctTree
{
	OctTreeNode octTree[];
};

layout(binding = 3) buffer OctTreeNodeCount
{
    uint octTreeNodeCount;
    uint totalNodeCount;
};

layout(binding = 4) buffer VoxVisitedBuf
{
    uint voxVisited [];
};

void main()
{
    // read from voxmap
    ivec3 vp = ivec3(gl_GlobalInvocationID.xyz);

    const uint voxSideCnt = conf.voxGridSideCount;

    if (vp.x >= voxSideCnt || vp.y >= voxSideCnt || vp.z >= voxSideCnt)
        return;

    const uint roff = conf.readOffset;
    const uint woff = conf.writeOffset;
    const uint lv = conf.lv;
    const uint vi = woff + vp.z * voxSideCnt * voxSideCnt + vp.y * voxSideCnt + vp.x;

    if (voxVisited[vi] == 1)
        return;

    uint t = atomicAdd(totalNodeCount, 1);

    // level start from 0
    // the 0 level is **not** the actual voxel level, but the one's childs contains the id of actual voxel data
    // the actural voxel id is stored in the voxmap and the data stores in other image buffers
    // e.g.: vox resolution was 32^3, the the 0 level has 16^3 nodes, each node contains 8 voxels
    uint nodes[8];
    uint res = 0;
    for (uint ii = 0; ii < 8; ii++)
    {
        uint cIdx = getVoxChildGridIdx(vp, voxSideCnt, ii);
        uint var = (lv == 0) ? voxMap[cIdx] : voxMediumMap[cIdx + roff];
        nodes[ii] = var;
        res |= (var ^ INVALID_OCT_IDX);
    }

    if (res > 0)
    {
        uint octNodeIdx = atomicAdd(octTreeNodeCount, 1);
        for (uint ii = 0; ii < 8; ii++)
        {
            octTree[octNodeIdx].childs[ii] = nodes[ii];
        }

        octTree[octNodeIdx].dataIdx = octNodeIdx;
        octTree[octNodeIdx].lv = lv;
        voxMediumMap[vi] = octNodeIdx;

        memoryBarrierBuffer();
    }

    voxVisited[vi] = 1;
}