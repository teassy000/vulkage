#include "common.h"
#include "math.h"

#include "mesh.h"
#include "scene.h"

enum class Scene_Enum : uint64_t
{
    RamdomScene = 0,
    MatrixScene,
    TenMatrixScene,
    SingleMeshScene,
    CornellBox,
};

static Scene_Enum se = Scene_Enum::TenMatrixScene;

void CreateRandomScene(Scene& scene)
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

        /*
        * NOTE: simplification for occlusion test
        meshDraws[i].pos[0] = 0.0f;

        meshDraws[i].pos[1] = 0.0f;
        meshDraws[i].pos[2] = (float)i + 2.0f;

        meshDraws[i].scale = 1.f / (float)i;
        */

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
        for (uint32_t lod = 0; lod < mesh.lodCount; lod++)
            meshletCount = std::max(meshletCount, mesh.lods[lod].meshletCount); // use the maximum one for current mesh

        meshletVisibilityCount += meshletCount;
    }

    scene.drawCount = drawCount;
    scene.drawDistance = drawDist;
    scene.meshletVisibilityCount = meshletVisibilityCount;
    scene.meshDraws.insert(scene.meshDraws.end(), meshDraws.begin(), meshDraws.end());
}

void CreateMatrixScene(Scene& scene)
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
        for (uint32_t lod = 0; lod < mesh.lodCount; lod++)
            meshletCount = std::max(meshletCount, mesh.lods[lod].meshletCount); // use the maximum one for current mesh

        meshletVisibilityCount += meshletCount;
    }

    scene.drawCount = drawCount;
    scene.drawDistance = drawDist;
    scene.meshletVisibilityCount = meshletVisibilityCount;
    scene.meshDraws.insert(scene.meshDraws.end(), meshDraws.begin(), meshDraws.end());
}

void CreateTenObjScene(Scene& scene)
{
    uint32_t drawCount = 10*10; // 500*500
    std::vector<MeshDraw> meshDraws(drawCount);

    float drawDist = 200;
    srand(42);
    uint32_t meshletVisibilityCount = 0;
    float basePos = -5.f;
    for (uint32_t i = 0; i < drawCount; ++i)
    {
        uint32_t meshIdx = rand() % scene.geometry.meshes.size();
        Mesh& mesh = scene.geometry.meshes[meshIdx];

        //-- NOTE: simplification for occlusion test
        meshDraws[i].pos[0] = float(i % 10) + basePos + 1.f;
        meshDraws[i].pos[1] = -1.f;
        meshDraws[i].pos[2] = float(i / 10) + basePos + 10.f;

        meshDraws[i].scale = 1.f;

        meshDraws[i].orit = quat(1, 0, 0, 0);
        meshDraws[i].meshIdx = meshIdx;
        meshDraws[i].vertexOffset = mesh.vertexOffset;
        meshDraws[i].meshletVisibilityOffset = meshletVisibilityCount;

        uint32_t meshletCount = 0;
        for (uint32_t lod = 0; lod < mesh.lodCount; lod++)
            meshletCount = std::max(meshletCount, mesh.lods[lod].meshletCount); // use the maximum one for current mesh

        meshletVisibilityCount += meshletCount;
    }

    scene.drawCount = drawCount;
    scene.drawDistance = drawDist;
    scene.meshletVisibilityCount = meshletVisibilityCount;
    scene.meshDraws.insert(scene.meshDraws.end(), meshDraws.begin(), meshDraws.end());
}

void CreateSingleMeshScene(Scene& _scene)
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
    for (uint32_t lod = 0; lod < mesh.lodCount; lod++)
        meshletCount = std::max(meshletCount, mesh.lods[lod].meshletCount); // use the maximum one for current mesh

    _scene.drawCount = 1;
    _scene.drawDistance = 100.f;
    _scene.meshletVisibilityCount = meshletCount;
    _scene.meshDraws.push_back(meshDraw);
}

bool loadScene(Scene& scene, const char** pathes, const uint32_t pathCount ,bool buildMeshlets)
{
    assert(pathCount);
    assert(pathes);

    //TODO: make a scene file that describe models and other things in the scene 
    for (uint32_t i = 0; i < pathCount; ++i)
    {
        bool rcm = false;
        rcm = loadMesh(scene.geometry, pathes[i], buildMeshlets);
        if (!rcm) {
            vkz::message(vkz::error, "Failed to load mesh %s", pathes[i]);
            return false;
        }

        assert(rcm);
    }

    if (scene.geometry.meshes.empty())
    {
        vkz::message(vkz::error, "No mesh was loaded!");
        return false;
    }

    switch (se)
    {
    case Scene_Enum::MatrixScene:
        CreateMatrixScene(scene);
        break;
    case Scene_Enum::SingleMeshScene:
        CreateSingleMeshScene(scene);
        break;
    case Scene_Enum::TenMatrixScene:
        CreateTenObjScene(scene);
        break;
    case Scene_Enum::RamdomScene:
    default:
        CreateRandomScene(scene);
        break;
    }

    return true;
}


