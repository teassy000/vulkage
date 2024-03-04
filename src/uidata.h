#pragma once

struct DebugProfilingData
{
    float cpuTime;
    float avgCpuTime;
    float gpuTime;
    float avgGpuTime;
    float cullEarlyTime;
    float cullLateTime;
    float drawEarlyTime;
    float drawLateTime;
    float pyramidTime;
    float uiTime;
    float waitTime;

    uint32_t primitiveCount;
    uint32_t meshletCount;
    uint32_t objCount;

    // use Million/Billion as units
    float triangleCount;
    float triangleEarlyCount;
    float triangleLateCount;
    float trianglesPerSec;
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
};

struct DebugLogicData
{
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
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