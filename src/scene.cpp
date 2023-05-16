#include "common.h"
#include "math.h"

#include "mesh.h"
#include "scene.h"

enum class Scene_Enum
{
    RamdomScene,
    MatrixScene,
    CornellBox,
};

static Scene_Enum se = Scene_Enum::MatrixScene;

void CreateRandomScene(Scene& scene)
{
    uint32_t drawCount = 1000000;
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
    uint32_t drawCount = 10000; // 100*100
    std::vector<MeshDraw> meshDraws(drawCount);

    float randomDist = 200;
    float drawDist = 200;
    srand(42);
    uint32_t meshletVisibilityCount = 0;
    float basePos = -50.f;
    for (uint32_t i = 0; i < drawCount; ++i)
    {
        uint32_t meshIdx = rand() % scene.geometry.meshes.size();
        Mesh& mesh = scene.geometry.meshes[meshIdx];


        //-- NOTE: simplification for occlusion test
        meshDraws[i].pos[0] = float(i % 100) + basePos + 2.f;

        meshDraws[i].pos[1] = -2.f;
        meshDraws[i].pos[2] = float(i / 100) + basePos + 2.f;

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

bool loadScene(Scene& scene, const char** pathes, const uint32_t pathCount ,bool buildMeshlets)
{
    assert(pathCount);
    assert(pathes);

    for (uint32_t i = 0; i < pathCount; ++i)
    {
        bool rcm = false;
        rcm = loadMesh(scene.geometry, pathes[i], buildMeshlets);
        if (!rcm) {
            printf("[Error]: Failed to load mesh %s", pathes[i]);
            return false;
        }

        assert(rcm);
    }

    if (scene.geometry.meshes.empty())
    {
        printf("[Error]: No mesh was loaded!");
        return false;
    }

    switch (se)
    {
    case Scene_Enum::MatrixScene:
        CreateMatrixScene(scene);
        break;
    case Scene_Enum::RamdomScene:
    default:
        CreateRandomScene(scene);
        break;
    }

    return true;
}


