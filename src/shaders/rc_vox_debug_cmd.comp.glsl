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

layout(binding = 0) readonly uniform Transform
{
    TransformData trans;
};

// writeonly
layout(binding = 1) writeonly buffer DrawCommand
{
    MeshDrawCommand drawCmd;
};

// read/write 
layout(binding = 2) buffer VoxDraws
{
    VoxDraw voxDraw [];
};


// readonly
layout(binding = 3) uniform sampler2D depthPyramid;
layout(binding = 4, R32UI) uniform readonly uimageBuffer wpos;
layout(binding = 5, RGBA8) uniform readonly imageBuffer in_albedo;


void main()
{
    uint di = gl_GlobalInvocationID.x;

    uint var = imageLoad(wpos, int(di)).x;
    vec3 abledo = imageLoad(in_albedo, int(di)).xyz;
    ivec3 wIdx = getWorld3DIdx(var, info.sceneSideCnt);
    vec4 ocenter = vec4(getProbeCenterPos(wIdx, info.sceneRadius, info.voxSideLen), 1.f);

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

        uint idx = atomicAdd(drawCmd.instanceCount, 1);
        
        drawCmd.firstIndex = 0;
        drawCmd.vertexOffset = 0;
        drawCmd.firstInstance = 0;
        drawCmd.local_x = 0;
        drawCmd.local_y = 0;
        drawCmd.local_z = 0;


        voxDraw[idx].pos = ocenter.xyz;
        voxDraw[idx].col = abledo;
    }
}

