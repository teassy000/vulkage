#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh.h"
#include "math.h"


layout(local_size_x = TASKGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(constant_id = 0) const bool LATE = false;
layout(constant_id = 1) const bool TASK = false;

layout(push_constant) uniform block 
{
    MeshDrawCull cull;
};

layout(binding = 0) readonly buffer Meshes
{
    Mesh meshes[];
};

layout(binding = 1) readonly buffer MeshDraws
{
    MeshDraw draws[];
};

layout(binding = 2) readonly buffer Transform
{
    TransformData transform;
};

layout(binding = 3) writeonly buffer DrawCommands
{
    MeshDrawCommand drawCmds[];
};

layout(binding = 3) writeonly buffer TaskCommands
{
    MeshTaskCommand taskCmds[];
};

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

    // the early cull only handle objects that visiable depends on last frame 
    if(!LATE && drawVisibility[di] == 0 && cull.enableOcclusion == 1)
        return;

    Mesh mesh = meshes[draws[di].meshIdx];

    vec4 center = transform.view * vec4(rotateQuat(mesh.center, draws[di].orit) * draws[di].scale + draws[di].pos, 1.0);
    float radius = mesh.radius * draws[di].scale;

    bool visible = true;
    visible = visible && (center.z * cull.frustum[1] + abs(center.x) * cull.frustum[0] > -radius);
	visible = visible && (center.z * cull.frustum[3] + abs(center.y) * cull.frustum[2] > -radius);
	
    visible = visible && (center.z + radius > cull.znear);
    visible = visible && (center.z - radius < cull.zfar);

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
    

    // early culling pass will setup the draw commands
    if(visible && (!LATE || cull.enableMeshletOcclusion == 1 || drawVisibility[di] == 0))
    {
        
        float lodDist = log2(max(1, distance(center.xyz, vec3(0)) - radius));
        uint lodIdx = cull.enableLod == 1 ? clamp(int(lodDist), 0, int(mesh.lodCount) - 1) : 0;
        MeshLod lod = mesh.lods[lodIdx];

        if(TASK)
        {
            uint taskGroupCount = (lod.meshletCount + TASKGP_SIZE - 1) / TASKGP_SIZE; // each task group handle TASKGP_SIZE meshlets
            uint dci = atomicAdd(drawCmdCount, taskGroupCount);

            uint meshletVisibilityOffset = draws[di].meshletVisibilityOffset;
            uint lateDrawVisibility = drawVisibility[di];

            for(uint i = 0; i < taskGroupCount; ++i)
            {
                taskCmds[dci + i].drawId = di;
                taskCmds[dci + i].taskOffset = lod.meshletOffset + i * TASKGP_SIZE;
                taskCmds[dci + i].taskCount = min(TASKGP_SIZE, lod.meshletCount - i * TASKGP_SIZE);
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

