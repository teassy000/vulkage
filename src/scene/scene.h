#pragma once

#include "assets/mesh.h"
#include "core/kage_math.h"
#include "core/kage.h"

#include <string>

struct alignas(16) MeshDraw
{
    vec3 pos;
    uint32_t meshIdx;

    vec3 scale;
    uint32_t vertexOffset; // same as mesh[meshIdx], for data locality

    quat orit;

    uint32_t meshletVisibilityOffset;
    uint32_t withAlpha{0};

    uint32_t albedoTex{0};
    uint32_t normalTex{0};
    uint32_t specularTex{0};
    uint32_t emissiveTex{0};
};

struct alignas(16) ImageInfo
{
    char name[128];
    uint32_t dataOffset;
    uint32_t dataSize;

    uint32_t w, h;
    uint32_t mipCount;
    uint32_t layerCount;
    kage::ResourceFormat format;
    bool isCubeMap;
};

struct Camera
{
    vec3 pos;
    quat orit;
    float fov;
};

struct Scene
{
    uint32_t drawCount;
    float    drawDistance; // remove for infinity far 
    uint32_t meshletVisibilityCount; // meshlet Count
    float    radius;

    uint32_t imageCount;
    uint32_t imageDataSize;

    Geometry geometry;
    std::vector<MeshDraw> meshDraws;

    std::vector<ImageInfo> images;
    std::vector<uint8_t> imageDatas;

    uint32_t cameraCount;
    std::vector<Camera> cameras;
};

bool loadScene(Scene& _scene, const std::vector<std::string>& _pathes, bool _buildMeshlets, bool _seamlessLod, bool _forceParse);
bool dumpScene(const Scene& scene, const char* path);

float calcRadius(const Scene& _scene);
