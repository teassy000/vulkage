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
    int  debugCascadeLevel = 0;
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

struct Dbg_RCBuild
{
    // brx
    float brx_tmin = .2f;
    float brx_tmax = 100.f;

    uint32_t brx_offset = 0;
    uint32_t brx_startCas = 0;
    uint32_t brx_endCas = 8;

    // radius
    float totalRadius = 50.f;
    uint32_t debug_type = 0;

    float probePosOffset[3] = { 0.f, 0.f, 0.f };
    float probeDebugScale = 0.05f;
    uint32_t rcLv = 0;
};

struct DebugFeatures
{
    Dbg_Common common;
    Dbg_Brixel brx;

    Dbg_RCBuild rcBuild;
};

struct DebugLogicData
{
    float posX, posY, posZ;
    float frontX, frontY, frontZ;
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