#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"

layout(local_size_x = TASKGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(constant_id = 0) const bool LATE = false;
layout(constant_id = 1) const bool TASK = false;
layout(constant_id = 2) const bool ALPHA_PASS = false;

layout(push_constant) uniform block 
{
    DrawCull cull;
};

// readonly
layout(binding = 0) readonly buffer Meshes
{
    Mesh meshes[];
};

layout(binding = 1) readonly buffer MeshDraws
{
    MeshDraw draws[];
};

layout(binding = 2) readonly uniform Transform
{
    TransformData trans;
};

// writeonly
layout(binding = 3) writeonly buffer DrawCommands
{
    MeshDrawCommand drawCmds[];
};

layout(binding = 3) writeonly buffer TaskCommands
{
    MeshTaskCommand taskCmds[];
};

// read/write 
layout(binding = 4) buffer DrawCommandCount
{
    uint drawCmdCount;
};

layout(binding = 5) buffer DrawVisibility
{
    uint drawVisibility[];
};

layout(binding = 6) uniform sampler2D depthPyramid;

void main()
{
    uint di = gl_GlobalInvocationID.x;

    MeshDraw draw = draws[di];
    if (draw.withAlpha > 0 && !ALPHA_PASS)
        return;

    if (ALPHA_PASS && draw.withAlpha == 0)
        return;

    // the early cull only handle objects that visiable depends on last frame 
    if (!LATE && drawVisibility[di] == 0 && cull.enableOcclusion == 1)
        return;

    Mesh mesh = meshes[draw.meshIdx];

    vec4 center = trans.cull_view * vec4(rotateQuat(mesh.center, draw.orit) * draw.scale + draw.pos, 1.0);
    float radius = mesh.radius * maxElem(draw.scale);

    bool visible = true;
    visible = visible && (center.z * cull.frustum[1] + abs(center.x) * cull.frustum[0] > -radius);
	visible = visible && (center.z * cull.frustum[3] + abs(center.y) * cull.frustum[2] > -radius);
	
    visible = visible && (center.z + radius > cull.znear);

    visible = visible || (cull.enableCull == 0);
    
    
    // do occlusion culling in late pass
    if(LATE && visible && cull.enableOcclusion == 1)
    {
        vec4 aabb;
        if(projectSphere(center.xyz, radius, cull.znear, cull.P00, cull.P11, aabb))
        {
            // the size in the render target
            float width = (aabb.z - aabb.x) * cull.pyramidWidth; 
            float height = (aabb.w - aabb.y) * cull.pyramidHeight;
    
            float level = floor(log2(max(width, height))); // smaller object would use lower level
    
            float depth = textureLod(depthPyramid, (aabb.xy + aabb.zw) * 0.5, level).x; // scene depth
            float depthSphere = cull.znear / (center.z - radius); 
            visible = visible && (depthSphere > depth); // nearest depth on sphere should less than the depth buffer
        }
    }

    // early pass, or force meshlet oc, or not draw will setup the draw commands if visiable
    if(visible && (!LATE || cull.enableMeshletOcclusion == 1 || drawVisibility[di] == 0))
    {
        float dist = max(length(center.xyz) - radius, 0);
        float threshold = dist * cull.lodErrorThreshold / maxElem(draw.scale);
        uint lodIdx = 0;
        for (uint ii = 0; ii < mesh.lodCount; ++ii){
            if (mesh.lods[ii].error < threshold){
                lodIdx = ii;
            }
        }

        MeshLod lod = cull.enableSeamlessLod == 1 ? mesh.seamlessLod : mesh.lods[lodIdx];

        if(TASK)
        {
            uint taskGroupCount = (lod.meshletCount + TASKGP_SIZE - 1) / TASKGP_SIZE; // each task group handle TASKGP_SIZE meshlets
            uint dci = atomicAdd(drawCmdCount, taskGroupCount);

            uint meshletVisibilityOffset = draw.meshletVisibilityOffset;
            uint lateDrawVisibility = drawVisibility[di];

            // the command for each command idx is the same
            for(uint i = 0; i < taskGroupCount; ++i)
            {
                taskCmds[dci + i].drawId = di;
                taskCmds[dci + i].taskOffset = lod.meshletOffset + i * TASKGP_SIZE;
                taskCmds[dci + i].taskCount = min(TASKGP_SIZE, lod.meshletCount - i * TASKGP_SIZE); // the last task group may have less than TASKGP_SIZE meshlets
                taskCmds[dci + i].lateDrawVisibility = lateDrawVisibility;
                taskCmds[dci + i].meshletVisibilityOffset = meshletVisibilityOffset + i * TASKGP_SIZE;
            }
        }
        else
        {
            uint dci = atomicAdd(drawCmdCount, 1);
            drawCmds[dci].drawId = di;
            drawCmds[dci].lateDrawVisibility = drawVisibility[di];
            drawCmds[dci].taskCount = lod.meshletCount;
            drawCmds[dci].taskOffset = lod.meshletOffset;
            drawCmds[dci].indexCount = lod.indexCount;
            drawCmds[dci].instanceCount = 1;
            drawCmds[dci].firstIndex = lod.indexOffset;
            drawCmds[dci].vertexOffset = mesh.vertexOffset;
            drawCmds[dci].firstInstance = 0;
            drawCmds[dci].local_x = (lod.meshletCount + TASKGP_SIZE - 1)/ TASKGP_SIZE; 
            drawCmds[dci].local_y = 1;
            drawCmds[dci].local_z = 1;
        }
    }

    // set dvb in late pass
    if(LATE) 
        drawVisibility[di] = visible ? 1 : 0; 
}

