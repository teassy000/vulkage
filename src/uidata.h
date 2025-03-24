#pragma once
#include "kage.h"
struct DebugProfilingData
{
    float avgCpuTime;
    float avgGpuTime;
    float cullEarlyTime;
    float cullLateTime;
    float drawEarlyTime;
    float drawLateTime;
    float pyramidTime;
    float uiTime;
    float deferredTime;
    float buildCascadeTime;

    uint32_t primitiveCount;
    uint32_t meshletCount;

    // use Million/Billion as units
    float triangleCount;
    float triangleEarlyCount;
    float triangleLateCount;
};

struct DebugRenderOptionsData
{
    bool meshShadingEnabled = true;
    bool objCullEnabled = true;
    bool lodEnabled = true;
    bool ocEnabled = true;
    bool meshletOcEnabled = true;
    bool taskSubmitEnabled = true;
    bool showPyramid = false;
    int  debugPyramidLevel = 0;
    int  debugCascadeLevel = 0;
    
    uint32_t debugBrixelType = 0;
    uint32_t startCas = 0;
    uint32_t endCas = 8;
    float sdfEps = 1.5f;
    float tmin = .5f;
    float tmax = 1000.f;
};

struct DebugLogicData
{
    float posX, posY, posZ;
    float frontX, frontY, frontZ;
};

struct DebugReources
{
    kage::ImageHandle brx_debug;
};

struct UIInput
{
    float mousePosx, mousePosy;

    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } mouseButtons;

    float width, height;
};