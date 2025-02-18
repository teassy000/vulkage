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
uint getVoxChildPos(ivec3 _vi, uint _voxLen, uint _childIdx)
{
    uint actualVoxLen = _voxLen * 2;

    ivec3 cPos = _vi * 2 + ivec3(_childIdx & 1u, (_childIdx >> 1) & 1u, (_childIdx >> 2) & 1u);

    uint pos = cPos.z * actualVoxLen * actualVoxLen + cPos.y * actualVoxLen + cPos.x;
    
    return pos;
}

void main()
{
    // read from voxmap
    ivec3 vp = ivec3(gl_GlobalInvocationID.xyz);

    const uint voxLen = conf.voxLen;
    const uint roff = conf.readOffset;
    const uint woff = conf.writeOffset;
    const uint lv = conf.lv;

    if (vp.x >= voxLen || vp.y >= voxLen || vp.z >= voxLen)
        return;

    // check if any child is valid
    uint nodes[8];
    uint res = 0;
    for (uint ii = 0; ii < 8; ii++)
    {
        uint cIdx = getVoxChildPos(vp, voxLen, ii);
        uint var = (lv == 0) ? voxMap[cIdx] : voxMediumMap[cIdx + roff];
        nodes[ii] = var;
        res |= (var ^ INVALID_OCT_IDX);
    }

    if(res > 0)
    {
        uint octNodeIdx = atomicAdd(octTreeNodeCount, 1);
        for (uint ii = 0; ii < 8; ii++)
        {
            octTree[octNodeIdx].childs[ii] = nodes[ii];
        }
        octTree[octNodeIdx].voxIdx = octNodeIdx;
        octTree[octNodeIdx].lv = lv;

        uint vi = woff + vp.z * voxLen * voxLen + vp.y * voxLen + vp.x;
        voxMediumMap[vi] = octNodeIdx;
    }
}