#pragma once

#include "mesh.h"
#include "math.h"

struct alignas(16) MeshDraw
{
    vec3 pos;
    float scale;
    quat orit;

    uint32_t meshIdx;
    uint32_t vertexOffset;
    uint32_t meshletVisibilityOffset;
};

struct Scene
{
    uint32_t drawCount;
    float    drawDistance; // remove for infinity far 
    uint32_t meshletVisibilityCount; // meshlet Count

    Geometry geometry;
    std::vector<MeshDraw> meshDraws;
};

bool loadScene(Scene& scene, const char** pathes, const uint32_t pathCount, bool buildMeshlets);
