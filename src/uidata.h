#pragma once

struct ProfilingData
{
    float cpuTime;
    float avgCpuTime;
    float gpuTime;
    float avgGpuTime;
    float cullTime;
    float drawTime;
    float lateRender;
    float lateCullTime;
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

struct RenderOptionsData
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

struct LogicData
{
    glm::vec3 cameraPos;
    glm::vec3 cameraLookAt;
};