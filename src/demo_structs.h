#pragma once
#include "common.h"
#include "math.h"

struct alignas(16) TransformData
{
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
};

struct alignas(16) MeshDrawCullVKZ
{
    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float lodBase, lodStep;
    float pyramidWidth, pyramidHeight;

    int32_t enableCull;
    int32_t enableLod;
    int32_t enableOcclusion;
    int32_t enableMeshletOcclusion;
};

struct alignas(16) GlobalsVKZ
{
    mat4 projection;

    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;
    float screenWidth, screenHeight;
    int enableMeshletOcclusion;
};

struct MeshDrawCommandVKZ
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


inline uint32_t previousPow2_new(uint32_t v)
{
    uint32_t r = 1;

    while (r < v)
        r <<= 1;
    r >>= 1;
    return r;
}

inline uint32_t calculateMipLevelCount_new(uint32_t width, uint32_t height)
{
    uint32_t result = 0;
    while (width > 1 || height > 1)
    {
        result++;
        width >>= 1;
        height >>= 1;
    }

    return result;
}

// left handed?
inline mat4 perspectiveProjection2(float fovY, float aspectWbyH, float zNear)
{
    float f = 1.0f / tanf(fovY / 2.0f);

    return mat4(
        f / aspectWbyH, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, zNear, 0.0f);
}

inline vec4 normalizePlane2(vec4 p)
{
    return p / glm::length(p);
}

inline uint32_t calculateMipLevelCount2(uint32_t width, uint32_t height)
{
    uint32_t result = 0;
    while (width > 1 || height > 1)
    {
        result++;
        width >>= 1;
        height >>= 1;
    }

    return result;
}