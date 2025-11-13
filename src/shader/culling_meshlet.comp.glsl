#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"

layout(local_size_x = MR_MESHLETGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(constant_id = 0) const bool LATE = false;
layout(constant_id = 1) const bool ALPHA_PASS = false;
layout(constant_id = 2) const bool SEAMLESS_LOD = false;

layout(push_constant) uniform block 
{
    Constants consts;
};

// read
layout(binding = 0) readonly buffer MeshletCmds
{
    MeshTaskCommand meshletCmds [];
};

layout(binding = 1) readonly buffer Meshes
{
    Mesh meshes [];
};

layout(binding = 2) readonly buffer MeshDraws
{
    MeshDraw meshDraws [];
};

layout(binding = 3) readonly uniform Transform
{
    TransformData trans;
};

layout(binding = 4) readonly buffer Clusters
{
    Cluster clusters [];
};

layout(binding = 4) readonly buffer Meshlets
{
    Meshlet meshlets [];
};

layout(binding = 5) readonly buffer MeshletCount
{
    IndirectDispatchCommand indirectCmdCnt;
};

// read/write
layout(binding = 6) buffer MeshletVisibility
{
    uint meshletVisibility [];
};

// write
layout(binding = 7) buffer MeshletPayloads
{
    MeshletPayload payloads[];
};

layout(binding = 8) buffer MeshletPayloadCount
{
    IndirectDispatchCommand meshletCount;
};

// read
layout(binding = 9) uniform sampler2D pyramid;


void main()
{
    uint mid = gl_WorkGroupID.x;
    uint mLocalId = gl_LocalInvocationID.x;

    MeshTaskCommand cmd = meshletCmds[mid];

    uint lateDrawVisibility = cmd.lateDrawVisibility;

    uint drawId = cmd.drawId;
    MeshDraw meshDraw = meshDraws[drawId];
    Mesh mesh = meshes[meshDraw.meshIdx];

    uint taskCount = cmd.taskCount;
    uint taskOffset = cmd.taskOffset;

    uint mi = mLocalId + taskOffset;

    uint mvIdx = cmd.meshletVisibilityOffset + mLocalId;

    bool skip = false;
    bool valid = (mLocalId < taskCount);
    bool visible = valid;

    if (!ALPHA_PASS && consts.enableMeshletOcclusion == 1 )
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
    vec3 cone_axis = vec3(0.0, 0.0, 0.0);
    float cone_cutoff = 0.f;
    vec3 ori_center = vec3(0.0, 0.0, 0.0);

    float maxScaleAxis = maxElem(meshDraw.scale);

    if (SEAMLESS_LOD)
    {
        vec3 p_center = rotateQuat(clusters[mi].p_c, meshDraw.orit) * meshDraw.scale + meshDraw.pos;

        vec3 s_center = rotateQuat(clusters[mi].s_c, meshDraw.orit) * meshDraw.scale + meshDraw.pos;

        float p_dist = max(length(p_center - trans.cull_cameraPos.xyz) - clusters[mi].p_r, 0);
        float p_threshold = p_dist * consts.lodErrorThreshold / maxScaleAxis;
        float s_dist = max(length(s_center - trans.cull_cameraPos.xyz) - clusters[mi].s_r, 0);
        float s_threshold = s_dist * consts.lodErrorThreshold / maxScaleAxis;

        cone_axis = vec3(int(clusters[mi].cone_axis[0]) / 127.0, int(clusters[mi].cone_axis[1]) / 127.0, int(clusters[mi].cone_axis[2]) / 127.0);

        cone_cutoff = int(clusters[mi].cone_cutoff) / 127.0;

        radius = clusters[mi].s_r * maxScaleAxis;
        ori_center = s_center;

        // culling based on parent bounds, shoule be visible if:
        // 1. self bounds has low error than threshold 
        // 2. parent bounds has high error than threshold
        bool cond = clusters[mi].s_err <= s_threshold && clusters[mi].p_err > p_threshold;
        visible = visible && cond;
    }
    else // normal lod pipeline
    {
        cone_axis = vec3(int(meshlets[mi].cone_axis[0]) / 127.0, int(meshlets[mi].cone_axis[1]) / 127.0, int(meshlets[mi].cone_axis[2]) / 127.0);
        cone_cutoff = int(meshlets[mi].cone_cutoff) / 127.0;

        ori_center = rotateQuat(meshlets[mi].center, meshDraw.orit) * meshDraw.scale + meshDraw.pos;
        radius = meshlets[mi].radius * maxScaleAxis;
    }

    center = (trans.cull_view * vec4(ori_center, 1.0)).xyz;
    // back face culling
    {
        vec3 ori_cone_axis = rotateQuat(cone_axis, meshDraw.orit);
        vec3 cone_axis = mat3(trans.cull_view) * ori_cone_axis;

        // meshlet level back face culling, here we culling in the world space
        bool culled = coneCull(ori_center, radius, ori_cone_axis, cone_cutoff, trans.cull_cameraPos.xyz);
        visible = visible && ((!culled) || (meshDraw.withAlpha > 0));
    }

    // frustum culling: left/right/top/bottom
    visible = visible && (center.z * consts.frustum[1] + abs(center.x) * consts.frustum[0] > -radius);
    visible = visible && (center.z * consts.frustum[3] + abs(center.y) * consts.frustum[2] > -radius);
    
    // near culling
    // note: not perform far culling to keep the same result with the old pipeline
    visible = visible && (center.z + radius > consts.znear);
    
    // occlussion culling
    if(LATE && consts.enableMeshletOcclusion == 1 && visible)
    {
        vec4 aabb;
        float P00 = trans.cull_proj[0][0];
        float P11 = trans.cull_proj[1][1];
        if(projectSphere(center.xyz, radius, consts.znear, P00, P11, aabb))
        {
            // the size in the render targetpyramidLevelHeight
            float width = (aabb.z - aabb.x) * consts.pyramidWidth; 
            float height = (aabb.w - aabb.y) * consts.pyramidHeight;

            float level = floor(log2(max(width, height))); // smaller object would use lower level

            float depth = textureLod(pyramid, (aabb.xy + aabb.zw) * 0.5, level).x; // scene depth
            float depthSphere = consts.znear / (center.z - radius); 
            visible = visible && (depthSphere > depth); // nearest depth on sphere should less than the depth buffer
        }
    }

    if(LATE && consts.enableMeshletOcclusion == 1 && valid) 
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
        uint payloadOffset = atomicAdd(meshletCount.count, 1u);
        
        payloads[payloadOffset] = MeshletPayload(
            mi // meshlet index
            , drawId // draw mid
        );
    }
}

