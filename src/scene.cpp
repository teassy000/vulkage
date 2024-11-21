#include "common.h"
#include "kage_math.h"

#include "mesh.h"
#include "scene.h"
#include "gltf_loader.h"

enum class Scene_Enum : uint64_t
{
    RamdomScene = 0,
    MatrixScene,
    TenMatrixScene,
    SingleMeshScene,
    CornellBox,
};

static Scene_Enum se = Scene_Enum::MatrixScene;

void CreateRandomScene(Scene& scene, bool _seamlessLod)
{
    uint32_t drawCount = 1'000'000;
    std::vector<MeshDraw> meshDraws(drawCount);

    float randomDist = 200;
    float drawDist = 200;
    srand(42);
    uint32_t meshletVisibilityCount = 0;

    for (uint32_t i = 0; i < drawCount; ++i)
    {
        uint32_t meshIdx = rand() % scene.geometry.meshes.size();
        Mesh& mesh = scene.geometry.meshes[meshIdx];

        meshDraws[i].pos[0] = (float(rand()) / RAND_MAX) * randomDist * 2.f - randomDist;
        meshDraws[i].pos[1] = (float(rand()) / RAND_MAX) * randomDist * 2.f - randomDist;
        meshDraws[i].pos[2] = (float(rand()) / RAND_MAX) * randomDist * 2.f - randomDist;
        meshDraws[i].scale = (float(rand()) / RAND_MAX) + 2.f; // 1.f ~ 2.f

        meshDraws[i].orit = glm::rotate(
            quat(1, 0, 0, 0)
            , glm::radians((float(rand()) / RAND_MAX) * 90.f)
            , vec3((float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1));
        meshDraws[i].meshIdx = meshIdx;
        meshDraws[i].vertexOffset = mesh.vertexOffset;
        meshDraws[i].meshletVisibilityOffset = meshletVisibilityCount;

        uint32_t meshletCount = 0;
        if (_seamlessLod) {
            meshletCount = std::max(meshletCount, mesh.seamlessLod.meshletCount);
        }
        else {
            for (uint32_t lod = 0; lod < mesh.lodCount; lod++)
                meshletCount = std::max(meshletCount, mesh.lods[lod].meshletCount); // use the maximum one for current mesh
        }
        meshletVisibilityCount += meshletCount;
    }

    scene.drawCount = drawCount;
    scene.drawDistance = drawDist;
    scene.meshletVisibilityCount = meshletVisibilityCount;
    scene.meshDraws.insert(scene.meshDraws.end(), meshDraws.begin(), meshDraws.end());
}

void CreateMatrixScene(Scene& scene, bool _seamlessLod)
{
    uint32_t drawCount = 250'000; // 500*500
    std::vector<MeshDraw> meshDraws(drawCount);

    float randomDist = 200;
    float drawDist = 200;
    srand(42);
    uint32_t meshletVisibilityCount = 0;
    float basePos = -250.f;
    for (uint32_t i = 0; i < drawCount; ++i)
    {
        uint32_t meshIdx = rand() % scene.geometry.meshes.size();
        Mesh& mesh = scene.geometry.meshes[meshIdx];


        //-- NOTE: simplification for occlusion test
        meshDraws[i].pos[0] = float(i % 500) + basePos + 2.f;

        meshDraws[i].pos[1] = -2.f;
        meshDraws[i].pos[2] = float(i / 500) + basePos + 2.f;

        meshDraws[i].scale = 1.f;

        meshDraws[i].orit = quat(1, 0, 0, 0);
        meshDraws[i].meshIdx = meshIdx;
        meshDraws[i].vertexOffset = mesh.vertexOffset;
        meshDraws[i].meshletVisibilityOffset = meshletVisibilityCount;

        uint32_t meshletCount = 0;
        if (_seamlessLod) {
            meshletCount = std::max(meshletCount, mesh.seamlessLod.meshletCount);
        }
        else {
            for (uint32_t lod = 0; lod < mesh.lodCount; lod++)
                meshletCount = std::max(meshletCount, mesh.lods[lod].meshletCount); // use the maximum one for current mesh
        }

        meshletVisibilityCount += meshletCount;
    }

    scene.drawCount = drawCount;
    scene.drawDistance = drawDist;
    scene.meshletVisibilityCount = meshletVisibilityCount;
    scene.meshDraws.insert(scene.meshDraws.end(), meshDraws.begin(), meshDraws.end());
}

void CreateTenObjScene(Scene& scene, bool _seamlessLod)
{
    uint32_t side = 10;
    uint32_t drawCount = side * side;
    std::vector<MeshDraw> meshDraws(drawCount);

    float drawDist = 200;
    srand(42);
    uint32_t meshletVisibilityCount = 0;
    float basePos =  ( - (float)side / 2.f);
    for (uint32_t i = 0; i < drawCount; ++i)
    {
        uint32_t meshIdx = rand() % scene.geometry.meshes.size();
        Mesh& mesh = scene.geometry.meshes[meshIdx];

        //-- NOTE: simplification for occlusion test
        meshDraws[i].pos[0] = float(i % side) + basePos + 1.f;
        meshDraws[i].pos[1] = -1.f;
        meshDraws[i].pos[2] = float(i / side) + basePos;

        meshDraws[i].scale = 1.f;

        meshDraws[i].orit = quat(1, 0, 0, 0);
        meshDraws[i].meshIdx = meshIdx;
        meshDraws[i].vertexOffset = mesh.vertexOffset;
        meshDraws[i].meshletVisibilityOffset = meshletVisibilityCount;

        uint32_t meshletCount = 0;
        if (_seamlessLod) {
            meshletCount = std::max(meshletCount, mesh.seamlessLod.meshletCount);
        }
        else {
            for (uint32_t lod = 0; lod < mesh.lodCount; lod++)
                meshletCount = std::max(meshletCount, mesh.lods[lod].meshletCount); // use the maximum one for current mesh
        }

        meshletVisibilityCount += meshletCount;
    }

    scene.drawCount = drawCount;
    scene.drawDistance = drawDist;
    scene.meshletVisibilityCount = meshletVisibilityCount;
    scene.meshDraws.insert(scene.meshDraws.end(), meshDraws.begin(), meshDraws.end());
}

void CreateSingleMeshScene(Scene& _scene, bool _seamlessLod)
{
    uint32_t drawCount = 1;
    MeshDraw meshDraw;

    Mesh& mesh = _scene.geometry.meshes[0];

    //-- NOTE: simplification for occlusion test
    meshDraw.pos[0] = 0.f;
    meshDraw.pos[1] = 0.f;
    meshDraw.pos[2] = 2.f;
    meshDraw.scale = 1.f;
    meshDraw.orit = quat(1, 0, 0, 0);
    meshDraw.meshIdx = 0;
    meshDraw.vertexOffset = mesh.vertexOffset;
    meshDraw.meshletVisibilityOffset = 0;

    uint32_t meshletCount = 0;
    if (_seamlessLod) {
        meshletCount = std::max(meshletCount, mesh.seamlessLod.meshletCount);
    }
    else {
        for (uint32_t lod = 0; lod < mesh.lodCount; lod++)
            meshletCount = std::max(meshletCount, mesh.lods[lod].meshletCount); // use the maximum one for current mesh
    }

    _scene.drawCount = 1;
    _scene.drawDistance = 100.f;
    _scene.meshletVisibilityCount = meshletCount;
    _scene.meshDraws.push_back(meshDraw);
}

bool loadObjScene(Scene& _scene, const char** _pathes, const uint32_t _pathCount, bool _buildMeshlets, bool _seamlessLod)
{
    assert(_pathCount);
    assert(_pathes);

    for (uint32_t i = 0; i < _pathCount; ++i)
    {
        bool rcm = false;
        rcm = loadObj(_scene.geometry, _pathes[i], _buildMeshlets, _seamlessLod);
        if (!rcm) {
            kage::message(kage::error, "Failed to load mesh %s", _pathes[i]);
            return false;
        }
        assert(rcm);
    }

    if (_scene.geometry.meshes.empty())
    {
        kage::message(kage::error, "No mesh was loaded!");
        return false;
    }

    switch (se)
    {
    case Scene_Enum::MatrixScene:
        CreateMatrixScene(_scene, _seamlessLod);
        break;
    case Scene_Enum::SingleMeshScene:
        CreateSingleMeshScene(_scene, _seamlessLod);
        break;
    case Scene_Enum::TenMatrixScene:
        CreateTenObjScene(_scene, _seamlessLod);
        break;
    case Scene_Enum::RamdomScene:
    default:
        CreateRandomScene(_scene, _seamlessLod);
        break;
    }
    return true;
}


const char* getExtension(const char* _path)
{
    const char* extension = strrchr(_path, '.');
    if (!extension || extension == _path) {
        kage::message(kage::error, "Invalid file format: %s", _path);
        return nullptr;
    }
    extension++; // Skip the dot
    return extension;
}

bool loadScene(Scene& _scene, const char** _pathes, const uint32_t _pathCount, bool _buildMeshlets, bool _seamlessLod)
{
    if (_pathCount == 0 || _pathes == nullptr)
    {
        kage::message(kage::error, "No path was provided!");
        return false;
    }

    // check the first file format
    const char* ext0 = getExtension(_pathes[0]);

    if (strcmp(ext0, "gltf") == 0 || strcmp(ext0, "glb") == 0)
    {
        return loadGltfScene(_scene, _pathes, _pathCount, _buildMeshlets, _seamlessLod);
    }

    if (strcmp(ext0, "obj") == 0)
    {
        return loadObjScene(_scene, _pathes, _pathCount, _buildMeshlets, _seamlessLod);
    }

    kage::message(kage::error, "Unsupported file format: %s", _pathes[0]);
    return false;
}


