#pragma once
#include "common.h"
#include "kage_math.h"
#include "uidata.h"
#include "camera.h"

struct alignas(16) TransformData
{
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
};

struct alignas(16) DrawCull
{
    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float lodBase, lodStep;
    float pyramidWidth, pyramidHeight;
    float lodErrorThreshold;

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
    float probeRangeRadius;
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


struct Dbg_Common
{
    bool meshShadingEnabled = true;
    bool objCullEnabled = true;
    bool lodEnabled = true;
    bool ocEnabled = true;
    bool meshletOcEnabled = true;
    bool taskSubmitEnabled = true;
    bool showPyramid = false;
    int  debugPyramidLevel = 0;
    float speed = 0.01f;
};

struct Dbg_Brixel
{
    uint32_t debugBrixelType = 0;
    uint32_t startCas = 0;
    uint32_t endCas = 8;
    float sdfEps = 1.5f;
    float tmin = .5f;
    float tmax = 1000.f;

    kage::ImageHandle presentImg;
    bool followCam = true;
};

struct Dbg_RadianceCascades
{
    // brx
    float brx_tmin = .2f;
    float brx_tmax = 100.f;

    uint32_t brx_offset = 0;
    uint32_t brx_startCas = 0;
    uint32_t brx_endCas = 8;
    float brx_sdfEps = 1.5f;

    // radius
    float totalRadius = 5.f;
    uint32_t idx_type = 0;
    uint32_t color_type = 0;

    float probePosOffset[3] = { 0.f, 0.f, 0.f };
    float probeDebugScale = 0.05f;
    uint32_t startCascade = 0;
    uint32_t cascadeCount = kage::k_rclv0_cascadeLv;

    bool followCam = true;
    bool pauseUpdate = false;
};

struct DebugFeatures
{
    Dbg_Common common;
    Dbg_Brixel brx;

    Dbg_RadianceCascades rcBuild;
};

struct DemoData
{
    UIInput input;
    DebugFeatures dbg_features;
    DebugProfilingData profiling;
    DebugLogicData logic;

    // constants
    Globals globals;
    DrawCull drawCull;

    // uniform
    TransformData trans;
};

inline vec4 normalizePlane(vec4 p)
{
    return p / glm::length(p);
}
