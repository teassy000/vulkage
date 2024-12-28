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
layout(constant_id = 1) const bool ALPHA_PASS = false;
layout(constant_id = 2) const bool SEAMLESS_LOD = false;

#define CULL 1

layout(local_size_x = TASKGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block 
{
    Globals globals;
};

// read
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

layout(binding = 3) readonly buffer Clusters
{
    Cluster clusters[];
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
    uint cmdId = 64 * gl_WorkGroupID.x + gl_WorkGroupID.y;
    MeshTaskCommand cmd = taskCmds[cmdId];
    uint drawId = cmd.drawId;

    uint lateDrawVisibility = cmd.lateDrawVisibility;
    
    MeshDraw meshDraw = meshDraws[drawId];
    Mesh mesh = meshes[meshDraw.meshIdx];

    uint taskCount = cmd.taskCount;
    uint taskOffset = cmd.taskOffset;

    uint mLocalId = gl_LocalInvocationID.x;
    uint mi = mLocalId + taskOffset;

    uint mvIdx = cmd.meshletVisibilityOffset + mLocalId;

#if CULL
    sharedCount = 0;
    barrier(); // wait for sharedCount to be initialized

    bool skip = false;
    bool valid = (mLocalId < taskCount);
    bool visible = valid;

    if (!ALPHA_PASS && globals.enableMeshletOcclusion == 1 )
    {
        // the meshlet visibility is using bit mask
        // mvIdx in range [0, meshletCount]
        // mvIdx >> 5 means divide by 32, so we can get the index of the visibility(which is uint32_t)
        // (1u << (mvIdx & 31) means get the bit index in the uint32_t
        uint mlvBit = (meshletVisibility[mvIdx >> 5] & (1u << (mvIdx & 31)));

        // early pass only handle objects that visiable depends on last frame
        if (!LATE && (mlvBit == 0))
        {
            visible = false;
        }

        // late pass only handle those meshlet *should* visiable but not draw in early pass
        if (LATE && mlvBit != 0 && lateDrawVisibility == 1)
        {
            skip = true;
        }
    }


    float radius = 0.0;
    vec3 center = vec3(0.0, 0.0, 0.0);


    if (SEAMLESS_LOD)
    {
        LodBounds p_bounds = clusters[mi].parent;
        p_bounds.center = rotateQuat(p_bounds.center, meshDraw.orit) * meshDraw.scale + meshDraw.pos;

        LodBounds s_bounds = clusters[mi].self;
        s_bounds.center = rotateQuat(s_bounds.center, meshDraw.orit) * meshDraw.scale + meshDraw.pos;

        float p_dist = max(length(p_bounds.center - trans.cameraPos) - p_bounds.radius, 0);
        float p_threshold = p_dist * globals.lodErrorThreshold / meshDraw.scale;
        float s_dist = max(length(s_bounds.center - trans.cameraPos) - s_bounds.radius, 0);
        float s_threshold = s_dist * globals.lodErrorThreshold / meshDraw.scale;

        bool cond = s_bounds.error <= s_threshold && p_bounds.error > p_threshold;

        radius = s_bounds.radius * meshDraw.scale;
        center = (trans.view * vec4(s_bounds.center, 1.0)).xyz;
        visible = visible && cond;
    }
    else // normal lod pipeline
    {
        vec3 axis = vec3(int(meshlets[mi].cone_axis[0]) / 127.0, int(meshlets[mi].cone_axis[1]) / 127.0, int(meshlets[mi].cone_axis[2]) / 127.0);
        vec3 ori_center = rotateQuat(meshlets[mi].center, meshDraw.orit) * meshDraw.scale + meshDraw.pos;

        float cone_cutoff = int(meshlets[mi].cone_cutoff) / 127.0;

        radius = meshlets[mi].radius * meshDraw.scale;
        center = (trans.view * vec4(ori_center, 1.0)).xyz;

        vec3 ori_cone_axis = rotateQuat(axis, meshDraw.orit);
        vec3 cone_axis = mat3(trans.view) * ori_cone_axis;
        // meshlet level back face culling, here we culling in the world space
        visible = visible && !coneCull(ori_center, radius, ori_cone_axis, cone_cutoff, trans.cameraPos);
    }

    // frustum culling: left/right/top/bottom
    visible = visible && (center.z * globals.frustum[1] + abs(center.x) * globals.frustum[0] > -radius);
    visible = visible && (center.z * globals.frustum[3] + abs(center.y) * globals.frustum[2] > -radius);
    
    // near culling
    // note: not perform far culling to keep the same result with the old pipeline
    visible = visible && (center.z + radius > globals.znear);
    
    // occlussion culling
    if(LATE && globals.enableMeshletOcclusion == 1 && visible)
    {
        vec4 aabb;
        float P00 = trans.proj[0][0];
        float P11 = trans.proj[1][1];
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

    if(LATE && globals.enableMeshletOcclusion == 1 && valid) 
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
        payload.meshletIndices[index] = cmdId | (mLocalId << 24);
    }

    payload.offset = cmd.taskOffset;
    payload.drawId = drawId;

    barrier(); // make sure all thread finished writing to sharedCount
    EmitMeshTasksEXT(sharedCount, 1, 1);
#else
    payload.meshletIndices[gl_LocalInvocationID.x] = mi;

    payload.drawId = drawId;
    EmitMeshTasksEXT(taskCount, 1, 1);
#endif
}
