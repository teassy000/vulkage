#pragma once
#include "core/kage.h"
struct DebugProfilingData
{
    float avgCpuTime;
    float avgGpuTime;
    float cullEarlyTime;
    float cullLateTime;
    float cullAlphaTime;
    float drawEarlyTime;
    float drawLateTime;
    float drawAlphaTime;
    float pyramidTime;
    float uiTime;
    float deferredTime;
    float buildCascadeTime;
    float mergeCascadeRayTime;
    float mergeCascadeProbeTime;
    float debugProbeGenTime;
    float debugProbeDrawTime;

    uint32_t primitiveCount;
    uint32_t meshletCount;

    // use Million/Billion as units
    float triangleCount;
    float triangleEarlyCount;
    float triangleLateCount;
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