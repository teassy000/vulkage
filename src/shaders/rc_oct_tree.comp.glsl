#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "rc_common.h"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform blocks
{
    OctTreeProcessConfig conf;
};

layout(binding = 0) readonly buffer VoxMap
{
	int voxMap[];
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
};


// 8 child
// zyx order from front to back, bottom to top, left to right
// [z][y][x]
// 0 = 000
// 1 = 001
// 2 = 010
// 3 = 011
// 4 = 100
// 5 = 101
// 6 = 110
// 7 = 111
uint getVoxChildPos(uint _vi, uint _voxLen, uint _chindIdx)
{
    ivec3 pPos = ivec3(_vi % _voxLen, (_vi / _voxLen) % _voxLen, _vi / (_voxLen * _voxLen));
    ivec3 cPos = pPos * 2 + ivec3(_chindIdx & 1, (_chindIdx >> 1) & 1, (_chindIdx >> 2) & 1);

    uint pos = cPos.z * _voxLen * _voxLen + cPos.y * _voxLen + cPos.x;
    return pos;
}

void main()
{
    // read from voxmap
    uint vi = gl_GlobalInvocationID.x;

    uint voxLen = conf.voxLen;
    ivec3 vpos = ivec3(vi % voxLen, (vi / voxLen) % voxLen, vi / (voxLen * voxLen));

    if(vpos.x >= voxLen || vpos.y >= voxLen || vpos.z >= voxLen)
        return;


    OctTreeNode node;
    uint res = 0;
    for (uint ii = 0; ii < 8; ii++)
    {
        uint cIdx = getVoxChildPos(vi, voxLen, ii);
        uint var = (conf.lv == 0) ? voxMap[cIdx] : voxMediumMap[cIdx];
        node.child[ii] = var;
        res |= (var ^ INVALID_OCT_IDX);
    }

    if(res != 0)
    {
        uint octNodeIdx = atomicAdd(octTreeNodeCount, 1);
        for (uint ii = 0; ii < 8; ii++)
        {
            octTree[octNodeIdx].child[ii] = node.child[ii];
        }
        octTree[octNodeIdx].voxIdx = octNodeIdx;
        octTree[octNodeIdx].isFinalLv = (conf.lv == 0) ? 1 : 0;

        voxMediumMap[vi] = octNodeIdx;
    }
}