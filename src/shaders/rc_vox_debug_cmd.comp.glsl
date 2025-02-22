#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"
#include "rc_common.h"

layout(local_size_x = TASKGP_SIZE, local_size_y = 1, local_size_z = 1) in;


layout(push_constant) uniform block
{
    VoxDebugConsts info;
};

layout(binding = 0) readonly buffer VoxMap
{
    uint voxmap [];
};

layout(binding = 1) readonly uniform Transform
{
    TransformData trans;
};

// writeonly
layout(binding = 2) writeonly buffer DrawCommand
{
    MeshDrawCommand drawCmd;
};

// read/write 
layout(binding = 3) buffer DrawPos
{
    vec3 drawPos [];
};

layout(binding = 4) uniform sampler2D depthPyramid;
layout(binding = 5, RGBA16F) uniform readonly imageBuffer wpos;

ivec3 getWorld3DIdx(uint _idx, uint _sideCnt)
{ 
    uint z = _idx / (_sideCnt * _sideCnt);
    uint y = (_idx % (_sideCnt * _sideCnt)) / _sideCnt;
    uint x = _idx % _sideCnt;

    return ivec3(x, y, z);
}

vec3 getCenterWorldPos(ivec3 _idx, float _sceneRadius, float _voxSideLen)
{
    // the voxel idx is from [0 ,sideCnt - 1]
    // the voxel in world space is [-_sceneRadius, _sceneRadius]
    vec3 pos = vec3(_idx) * _voxSideLen - vec3(_sceneRadius) - vec3(_voxSideLen) * 0.5f;
    return pos;
}


void main()
{
    uint di = gl_GlobalInvocationID.x;

    if (voxmap[di] == INVALID_VOX_ID)
        return;

    uint idx = voxmap[di];
    ivec3 wIdx = getWorld3DIdx(idx, info.sceneSideCnt);
    vec4 ocenter = vec4(getCenterWorldPos(wIdx, info.sceneRadius, info.voxSideLen), 1.f);

    //vec4 ocenter = imageLoad(wpos, int(di));
    vec4 center = trans.view * ocenter;

    float radius = info.voxRadius;

    bool visible = true;
    visible = visible && (center.z * info.frustum[1] + abs(center.x) * info.frustum[0] > -radius);
	visible = visible && (center.z * info.frustum[3] + abs(center.y) * info.frustum[2] > -radius);
	
    visible = visible && (center.z + radius > info.znear);

    if(visible)
    {
        vec4 aabb;
        if(projectSphere(center.xyz, radius, info.znear, info.P00, info.P11, aabb))
        {
            // the size in the render target
            float width = (aabb.z - aabb.x) * info.pyramidWidth; 
            float height = (aabb.w - aabb.y) * info.pyramidHeight;
    
            float level = floor(log2(max(width, height))); // smaller object would use lower level
    
            float depth = textureLod(depthPyramid, (aabb.xy + aabb.zw) * 0.5, level).x; // scene depth
            float depthSphere = info.znear / (center.z - radius); 
            visible = visible && (depthSphere > depth); // nearest depth on sphere should less than the depth buffer
        }
    }

    if(visible)
    {
        drawCmd.drawId = 0;
        drawCmd.taskOffset = 0;
        drawCmd.taskCount = 1;
        drawCmd.lateDrawVisibility = 0;
        drawCmd.indexCount = 36;

        uint var = atomicAdd(drawCmd.instanceCount, 1);
        
        drawCmd.firstIndex = 0;
        drawCmd.vertexOffset = 0;
        drawCmd.firstInstance = 0;
        drawCmd.local_x = 0;
        drawCmd.local_y = 0;
        drawCmd.local_z = 0;

        uint idx = var - 1;
        drawPos[idx] = ocenter.xyz;
    }
}

