#pragma once
#include "common.h"
#include "kage_math.h"
#include "uidata.h"
#include "camera.h"

struct alignas(16) TransformData
{
    float view[16];
    mat4 proj;
    vec3 cameraPos;
};

struct alignas(16) MeshDrawCull
{
    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float lodBase, lodStep;
    float pyramidWidth, pyramidHeight;

    int32_t enableCull;
    int32_t enableLod;
    int32_t enableSeamlessLod;
    int32_t enableOcclusion;
    int32_t enableMeshletOcclusion;
};

struct alignas(16) Globals
{
    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;
    float screenWidth, screenHeight;
    int enableMeshletOcclusion;
    float lodErrorThreshold;
};

struct MeshDrawCommand
{
    uint32_t    drawId;
    uint32_t    taskOffset;
    uint32_t    taskCount;
    uint32_t    lateDrawVisibility;

    // struct VkDrawIndexedIndirectCommand
    uint32_t    indexCount;
    uint32_t    instanceCount;
    uint32_t    firstIndex;
    uint32_t    vertexOffset;
    uint32_t    firstInstance;

    // struct VkDrawMeshTasksIndirectCommandEXT
    uint32_t    local_x;
    uint32_t    local_y;
    uint32_t    local_z;
};

struct DemoData
{
    UIInput input;
    DebugRenderOptionsData renderOptions;
    DebugProfilingData profiling;
    DebugLogicData logic;

    // constants
    Globals globals;
    MeshDrawCull drawCull;

    // uniform
    TransformData trans;
};


// left handed?
inline mat4 perspectiveProjection(float fovY, float aspectWbyH, float zNear)
{
    float f = 1.0f / tanf(fovY / 2.0f);

    return mat4(
        f / aspectWbyH, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, zNear, 0.0f);
}

inline vec4 normalizePlane(vec4 p)
{
    return p / glm::length(p);
}
