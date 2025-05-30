#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"
#include "rc_common.h"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block
{
    ProbeDebugCmdConsts consts;
};

layout(binding  = 0) readonly uniform Transform
{
    RadianceCascadesTransform trans;
};

// writeonly
layout(binding = 1) writeonly buffer DrawCommand
{
    MeshDrawCommand drawCmd;
};

// read/write
layout(binding = 2) buffer ProbeDraws
{
    ProbeDraw probeDraws [];
};

// readonly
layout(binding = 3) uniform sampler2D depthPyramid;


void main()
{
    ivec3 id = ivec3(gl_GlobalInvocationID.xyz);

    float rc_radius = consts.rcRadius;
    float probeSideLen = consts.probeSideLen;
    uint layerOffset = consts.layerOffset;
    float radius = consts.sphereRadius;
    vec3 ocenter = getProbeCenterPos(id, rc_radius, probeSideLen);
    ocenter += vec3(consts.posOffsets[0], consts.posOffsets[1], consts.posOffsets[2]);
    ocenter += trans.cameraPos;
    vec4 center = trans.view * vec4(ocenter, 1.f);


    bool visible = true;
    visible = visible && (center.z * consts.frustum[1] + abs(center.x) * consts.frustum[0] > -radius);
    visible = visible && (center.z * consts.frustum[3] + abs(center.y) * consts.frustum[2] > -radius);

    visible = visible && (center.z + radius > consts.znear);

    if (visible)
    {
        vec4 aabb;
        if (projectSphere(center.xyz, radius, consts.znear, consts.P00, consts.P11, aabb))
        {
            // the size in the render target
            float width = (aabb.z - aabb.x) * consts.pyramidWidth;
            float height = (aabb.w - aabb.y) * consts.pyramidHeight;

            float level = floor(log2(max(width, height))); // smaller object would use lower level

            float depth = textureLod(depthPyramid, (aabb.xy + aabb.zw) * 0.5, level).x; // scene depth
            float depthSphere = consts.znear / (center.z - radius);
            visible = visible && (depthSphere > depth); // nearest depth on sphere should less than the depth buffer
        }
    }

    if (visible)
    {
        drawCmd.drawId = 0;
        drawCmd.taskOffset = 0;
        drawCmd.taskCount = 1;
        drawCmd.lateDrawVisibility = 0;
        drawCmd.indexCount = consts.idxCnt;

        uint idx = atomicAdd(drawCmd.instanceCount, 1);

        drawCmd.firstIndex = 0;
        drawCmd.vertexOffset = 0;
        drawCmd.firstInstance = 0;
        drawCmd.local_x = 0;
        drawCmd.local_y = 0;
        drawCmd.local_z = 0;

        probeDraws[idx].pos = ocenter.xyz;
        probeDraws[idx].idx = ivec3(id.x, id.y, id.z + layerOffset);
    }
}