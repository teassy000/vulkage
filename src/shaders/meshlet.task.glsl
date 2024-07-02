#version 450

#extension GL_ARB_shader_draw_parameters: require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_mesh_shader: require
#extension GL_KHR_shader_subgroup_ballot: require


#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"

layout(constant_id = 0) const bool LATE = false;
layout(constant_id = 1) const bool TASK = false;

#define CULL 1

layout(local_size_x = TASKGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block 
{
    Globals globals;
};

// read
layout(binding = 0) readonly buffer DrawCommands 
{
    MeshDrawCommand drawCmds[];
};

layout(binding = 0) readonly buffer TaskCommands
{
    MeshTaskCommand taskCmds[];
};

layout(binding = 1) readonly buffer Meshes
{
    Mesh meshes[];
};

layout(binding = 2) readonly buffer MeshDraws
{
    MeshDraw meshDraws[];
};

layout(binding = 3) readonly buffer Meshlets
{
    Meshlet meshlets[];
};

layout(binding = 6) readonly uniform Transform
{
    TransformData trans;
};

// read/write
layout(binding = 7) buffer MeshletVisibility
{
    uint meshletVisibility[];
};

// read
layout(binding = 8) uniform sampler2D pyramid;

taskPayloadSharedEXT TaskPayload payload;

#if CULL
shared int sharedCount;
#endif

bool coneCullApex(vec3 cone_apex, vec3 cone_axis, float cone_cutoff, vec3 camera_position)
{
    return dot(normalize(cone_apex - camera_position), cone_axis) >= cone_cutoff;
}

void main()
{
    uint taskId = 0;
    if(TASK)
    {
        taskId = 64 * gl_WorkGroupID.x + gl_WorkGroupID.y;
    }

    uint drawId = TASK ? taskCmds[taskId].drawId : drawCmds[gl_DrawIDARB].drawId;
    uint lateDrawVisibility =  TASK ? taskCmds[taskId].lateDrawVisibility : drawCmds[gl_DrawIDARB].lateDrawVisibility;
    
    MeshDraw meshDraw = meshDraws[drawId];
    Mesh mesh = meshes[meshDraw.meshIdx];

    uint taskCount = TASK ? taskCmds[taskId].taskCount : drawCmds[gl_DrawIDARB].taskCount;
    uint taskOffset = TASK ? taskCmds[taskId].taskOffset : drawCmds[gl_DrawIDARB].taskOffset;
    
    uint mLocalId = TASK ? gl_LocalInvocationID.x : gl_GlobalInvocationID.x;
    uint mi = mLocalId + taskOffset;


    uint mvIdx = (TASK ? taskCmds[taskId].meshletVisibilityOffset : meshDraw.meshletVisibilityOffset) + mLocalId;

#if CULL
    vec3 axis = vec3( int(meshlets[mi].cone_axis[0]) / 127.0, int(meshlets[mi].cone_axis[1]) / 127.0, int(meshlets[mi].cone_axis[2]) / 127.0); 
    
    vec3 ori_cone_axis = rotateQuat(axis, meshDraw.orit);
    vec3 ori_center = rotateQuat(meshlets[mi].center, meshDraw.orit) * meshDraw.scale + meshDraw.pos;

    vec3 cone_axis = normalize(trans.view * vec4(ori_cone_axis, 1.0)).xyz;
    vec3 center = (trans.view * vec4(ori_center, 1.0)).xyz;

    float radius = meshlets[mi].radius * meshDraw.scale;
    float cone_cutoff = int(meshlets[mi].cone_cutoff) / 127.0;
    vec3 cameraPos = trans.cameraPos;

    sharedCount = 0;
    barrier();

    bool skip = false;
    bool visible = (mLocalId < taskCount);

    if(globals.enableMeshletOcclusion == 1)
    {
        uint mlvBit = (meshletVisibility[mvIdx >> 5] & (1u << (mvIdx & 31)));
        if (!LATE && (mlvBit == 0)) // deny invisible object in early pass
        {
            visible = false;
        }
        
        if ( LATE && mlvBit != 0 && lateDrawVisibility == 1)
        {
            skip = true;
        }
    }

    
    // meshlet level back face culling, here we culling in the world space
    visible = visible && !coneCull(ori_center, radius, ori_cone_axis, cone_cutoff, cameraPos);
    
    // frustum culling: left/right/top/bottom
    visible = visible && (center.z * globals.frustum[1] + abs(center.x) * globals.frustum[0] > -radius);
    visible = visible && (center.z * globals.frustum[3] + abs(center.y) * globals.frustum[2] > -radius);
    
    // near culling
    // note: not going to perform far culling to keep same result with triditional pipeline
    visible = visible && (center.z + radius > globals.znear);
    
    // occlussion culling
    if(LATE && globals.enableMeshletOcclusion == 1 && visible)
    {
        vec4 aabb;
        float P00 = globals.projection[0][0];
        float P11 = globals.projection[1][1];
        if(projectSphere(center.xyz, radius, globals.znear, P00, P11, aabb))
        {
            // the size in the render targetpyramidLevelHeight
            float width = (aabb.z - aabb.x) * globals.pyramidWidth; 
            float height = (aabb.w - aabb.y) * globals.pyramidHeight;

            float level = floor(log2(max(width, height))); // smaller object would use lower level

            float depth = textureLod(pyramid, (aabb.xy + aabb.zw) * 0.5, level).x; // scene depth
            float depthSphere = globals.znear / (center.z - radius); 
            visible = visible && (depthSphere > depth); // nearest depth on sphere should less than the depth buffer
        }
    }

    //TODO: wondering if bitwise operation which may behaves worse than the `unit` version(which would consume around 32 times memory size)
    if(LATE && globals.enableMeshletOcclusion == 1) 
    {
        if(visible)
        {
            atomicOr(meshletVisibility[mvIdx >> 5], 1u << (mvIdx & 31));
        }
        else
        {
            atomicAnd(meshletVisibility[mvIdx >> 5], ~(1u << (mvIdx & 31)));
        }
    }

    if( visible && !skip)
    {
        uint index = atomicAdd(sharedCount, 1);
        payload.meshletIndices[index] = mi;
    }

    payload.drawId = drawId;

    barrier();
    EmitMeshTasksEXT(sharedCount, 1, 1);
#else
    payload.meshletIndices[gl_LocalInvocationID.x] = mi;

    payload.drawId = drawId;
    EmitMeshTasksEXT(taskCount, 1, 1);
#endif
}
