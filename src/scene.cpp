#include "common.h"
#include "kage_math.h"

#include "mesh.h"
#include "scene.h"
#include "gltf_loader.h"
#include "file_helper.h"

enum class Scene_Enum : uint64_t
{
    RamdomScene = 0,
    MatrixScene,
    TenMatrixScene,
    SingleMeshScene,
    CornellBox,
};

struct SceneBiref
{
    // geometry brief
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t meshletCount;
    uint32_t clusterCount;
    uint32_t meshletDataCount;
    uint32_t meshCount;

    // draw brief
    uint32_t drawCount;
    float drawDistance;
    uint32_t meshletVisibilityCount;
    uint32_t imageCount;
    uint32_t imageDataSize;
    float    radius;
    
    // camera
    uint32_t cameraCount;
};

enum class SceneDumpDataTags : uint32_t
{
    // geometry
    vertex,
    index,
    meshlet,
    cluster,
    meshlet_data,
    mesh,
    // draw
    mesh_draw,
    image_info,
    image_data,
    // camera
    camera,
};

static Scene_Enum se = Scene_Enum::TenMatrixScene;

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
        meshDraws[i].scale = vec3(float(rand()) / RAND_MAX) + 2.f; // 1.f ~ 2.f

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

        meshDraws[i].scale = vec3(1.f);

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
    float basePos =  ( - (float)side);
    for (uint32_t i = 0; i < drawCount; ++i)
    {
        uint32_t meshIdx = rand() % scene.geometry.meshes.size();
        Mesh& mesh = scene.geometry.meshes[meshIdx];

        //-- NOTE: simplification for occlusion test
        meshDraws[i].pos[0] = float(i % side) * 2.f + basePos + 1.f;
        meshDraws[i].pos[1] = -1.f;
        meshDraws[i].pos[2] = float(i / side) * 2.f + basePos;

        meshDraws[i].scale = vec3(float(rand()) / RAND_MAX) + 2.f; // 1.f ~ 2.f;

        meshDraws[i].orit = glm::rotate(
            quat(1, 0, 0, 0)
            , glm::radians((float(rand()) / RAND_MAX) * 360.f)
            , vec3(0, 1, 0));

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
    meshDraw.scale = vec3(1.f);
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

bool loadObjScene(Scene& _scene, const std::vector<std::string>& _pathes, bool _buildMeshlets, bool _seamlessLod)
{
    assert(!_pathes.empty());

    for (uint32_t i = 0; i < _pathes.size(); ++i)
    {
        bool rcm = false;
        const std::string& path = _pathes[i];
        rcm = loadObj(_scene.geometry, path.c_str(), _buildMeshlets, _seamlessLod);
        if (!rcm) {
            kage::message(kage::error, "Failed to load mesh %s", path.c_str());
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

    _scene.radius = calcRadius(_scene);
    return true;
}

static size_t writeToFile(SceneDumpDataTags _tag, void* _data, size_t _stride , size_t _count, FILE* _file)
{
    fwrite(&_tag, sizeof(SceneDumpDataTags), 1, _file);
    return fwrite(_data, _stride, _count, _file);
}

static void printBrief(const SceneBiref& _brief, size_t _size)
{
    // print brief
    kage::message(kage::info, "Scene brief:");
    kage::message(kage::info, "vertex count: %d", _brief.vertexCount);
    kage::message(kage::info, "index count: %d", _brief.indexCount);
    kage::message(kage::info, "meshlet count: %d", _brief.meshletCount);
    kage::message(kage::info, "cluster count: %d", _brief.clusterCount);
    kage::message(kage::info, "meshlet data count: %d", _brief.meshletDataCount);
    kage::message(kage::info, "mesh count: %d", _brief.meshCount);
    kage::message(kage::info, "draw count: %d", _brief.drawCount);
    kage::message(kage::info, "draw distance: %f", _brief.drawDistance);
    kage::message(kage::info, "meshlet visibility count: %d", _brief.meshletVisibilityCount);
    kage::message(kage::info, "image count: %d", _brief.imageCount);
    kage::message(kage::info, "image data size: %d", _brief.imageDataSize);
    kage::message(kage::info, "camera count: %d", _brief.cameraCount);
    kage::message(kage::info, "scene radius: %f", _brief.radius);

    // total data size
    kage::message(kage::info, "Total data size: %d", _size);
}

bool dumpScene(const Scene& _scene, const char* _path)
{
    uint32_t dataSize = 0;
    const char* ext = getExtension(_path);
    if (strcmp(ext, "scene") != 0)
    {   
        kage::message(kage::error, "Invalid file format: %s", _path);
        return false;
    }


    FILE* file = fopen(_path, "wb");
    if (!file)
    {
        kage::message(kage::error, "Failed to open file: %s", _path);
        return false;
    }

    SceneBiref brief;
    brief.vertexCount = (uint32_t)_scene.geometry.vertices.size();
    brief.indexCount = (uint32_t)_scene.geometry.indices.size();
    brief.meshletCount = (uint32_t)_scene.geometry.meshlets.size();
    brief.clusterCount = (uint32_t)_scene.geometry.clusters.size();
    brief.meshletDataCount = (uint32_t)_scene.geometry.meshletdata.size();
    brief.meshCount = (uint32_t)_scene.geometry.meshes.size();

    brief.drawCount = _scene.drawCount;
    brief.drawDistance = _scene.drawDistance;
    brief.meshletVisibilityCount = _scene.meshletVisibilityCount;
    brief.imageCount = (uint32_t)_scene.images.size();
    brief.imageDataSize = (uint32_t)_scene.imageDatas.size();
    brief.cameraCount = (uint32_t)_scene.cameras.size();
    brief.radius = _scene.radius;

    fwrite(&brief, sizeof(SceneBiref), 1, file);


    size_t size = 0;
    // geometry
    size += writeToFile(SceneDumpDataTags::vertex, (void*)_scene.geometry.vertices.data(), sizeof(Vertex), _scene.geometry.vertices.size(), file);
    size += writeToFile(SceneDumpDataTags::index, (void*)_scene.geometry.indices.data(), sizeof(uint32_t), _scene.geometry.indices.size(), file);
    size += writeToFile(SceneDumpDataTags::meshlet, (void*)_scene.geometry.meshlets.data(), sizeof(Meshlet), _scene.geometry.meshlets.size(), file);
    size += writeToFile(SceneDumpDataTags::cluster, (void*)_scene.geometry.clusters.data(), sizeof(Cluster), _scene.geometry.clusters.size(), file);
    size += writeToFile(SceneDumpDataTags::meshlet_data, (void*)_scene.geometry.meshletdata.data(), sizeof(uint32_t), _scene.geometry.meshletdata.size(), file);
    size += writeToFile(SceneDumpDataTags::mesh, (void*)_scene.geometry.meshes.data(), sizeof(Mesh), _scene.geometry.meshes.size(), file);

    // draw
    size += writeToFile(SceneDumpDataTags::mesh_draw, (void*)_scene.meshDraws.data(), sizeof(MeshDraw), _scene.meshDraws.size(), file);
    size += writeToFile(SceneDumpDataTags::image_info, (void*)_scene.images.data(), sizeof(ImageInfo), _scene.images.size(), file);
    size += writeToFile(SceneDumpDataTags::image_data, (void*)_scene.imageDatas.data(), sizeof(uint8_t), _scene.imageDatas.size(), file);

    // camera
    size += writeToFile(SceneDumpDataTags::camera, (void*)_scene.cameras.data(), sizeof(Camera), _scene.cameras.size(), file);

    fclose(file);

    printBrief(brief, size);

    return true;
}


float calcRadius(const Scene& _scene)
{
    float radius = 0.f;

    for (const MeshDraw& md : _scene.meshDraws)
    {
        float r = _scene.geometry.meshes[md.meshIdx].radius;
        r = length(md.pos) + r * std::max(md.scale.x, std::max(md.scale.y, md.scale.z));

        radius = std::max(radius, r);
    }

    return radius;
}

void* getEntryPoint(Scene& _scene, SceneDumpDataTags _tag)
{
    switch (_tag)
    {
    case SceneDumpDataTags::vertex:
        return (void*)_scene.geometry.vertices.data();
    case SceneDumpDataTags::index:
        return (void*)_scene.geometry.indices.data();
    case SceneDumpDataTags::meshlet:
        return (void*)_scene.geometry.meshlets.data();
    case SceneDumpDataTags::cluster:
        return (void*)_scene.geometry.clusters.data();
    case SceneDumpDataTags::meshlet_data:
        return (void*)_scene.geometry.meshletdata.data();
    case SceneDumpDataTags::mesh:
        return (void*)_scene.geometry.meshes.data();
    case SceneDumpDataTags::mesh_draw:
        return (void*)_scene.meshDraws.data();
    case SceneDumpDataTags::image_info:
        return (void*)_scene.images.data();
    case SceneDumpDataTags::image_data:
        return (void*)_scene.imageDatas.data();
    case SceneDumpDataTags::camera:
        return (void*)_scene.cameras.data();
    default:
        return nullptr;
    }
}

size_t getStride(SceneDumpDataTags _tag)
{
    switch (_tag)
    {
    case SceneDumpDataTags::vertex:
        return sizeof(Vertex);
    case SceneDumpDataTags::index:
        return sizeof(uint32_t);
    case SceneDumpDataTags::meshlet:
        return sizeof(Meshlet);
    case SceneDumpDataTags::cluster:
        return sizeof(Cluster);
    case SceneDumpDataTags::meshlet_data:
        return sizeof(uint32_t);
    case SceneDumpDataTags::mesh:
        return sizeof(Mesh);
    case SceneDumpDataTags::mesh_draw:
        return sizeof(MeshDraw);
    case SceneDumpDataTags::image_info:
        return sizeof(ImageInfo);
    case SceneDumpDataTags::image_data:
        return sizeof(uint8_t);
    case SceneDumpDataTags::camera:
        return sizeof(Camera);
    default:
        return 0;
    }
}

size_t getElementCount(Scene& _scene, SceneDumpDataTags _tag)
{
    switch (_tag)
    {
    case SceneDumpDataTags::vertex:
        return _scene.geometry.vertices.size();
    case SceneDumpDataTags::index:
        return _scene.geometry.indices.size();
    case SceneDumpDataTags::meshlet:
        return _scene.geometry.meshlets.size();
    case SceneDumpDataTags::cluster:
        return _scene.geometry.clusters.size();
    case SceneDumpDataTags::meshlet_data:
        return _scene.geometry.meshletdata.size();
    case SceneDumpDataTags::mesh:
        return _scene.geometry.meshes.size();
    case SceneDumpDataTags::mesh_draw:
        return _scene.meshDraws.size();
    case SceneDumpDataTags::image_info:
        return _scene.images.size();
    case SceneDumpDataTags::image_data:
        return _scene.imageDatas.size();
    case SceneDumpDataTags::camera:
        return _scene.cameras.size();
    default:
        return 0;
    }
}


bool loadSceneDump(Scene& _scene, const char* _path)
{
    const char* ext = getExtension(_path);
    if (strcmp(ext, "scene") != 0)
    {
        kage::message(kage::warning, "Invalid file format: %s", _path);
        return false;
    }
    FILE* file = fopen(_path, "rb");
    if (!file)
    {
        kage::message(kage::warning, "Failed to open file: %s", _path);
        return false;
    }

    SceneBiref brief;
    fread(&brief, sizeof(SceneBiref), 1, file);
    _scene.geometry.vertices.resize(brief.vertexCount);
    _scene.geometry.indices.resize(brief.indexCount);
    _scene.geometry.meshlets.resize(brief.meshletCount);
    _scene.geometry.clusters.resize(brief.clusterCount);
    _scene.geometry.meshletdata.resize(brief.meshletDataCount);
    _scene.geometry.meshes.resize(brief.meshCount);
    _scene.drawCount = brief.drawCount;
    _scene.drawDistance = brief.drawDistance;
    _scene.meshletVisibilityCount = brief.meshletVisibilityCount;
    _scene.imageCount = brief.imageCount;
    _scene.imageDataSize = brief.imageDataSize;
    _scene.meshDraws.resize(brief.drawCount);
    _scene.images.resize(brief.imageCount);
    _scene.imageDatas.resize(brief.imageDataSize);
    _scene.cameras.resize(brief.cameraCount);
    _scene.radius = brief.radius;

    size_t size = 0;
    while (true)
    {
        if (feof(file))
            break;

        SceneDumpDataTags tag;
        fread(&tag, sizeof(SceneDumpDataTags), 1, file);


        size_t stride = getStride(tag);
        size_t count = getElementCount(_scene, tag);

        if (0 == count)
            continue;

        void* data = getEntryPoint(_scene, tag);
        if (data == nullptr)
        {
            kage::message(kage::error, "Invalid tag: %d", tag);
            continue;
        }

        size += fread(data, stride, count, file);
    }

    fclose(file);
    
    printBrief(brief, size);

    return true;
}

bool loadScene(Scene& _scene, const std::vector<std::string>& _pathes, bool _buildMeshlets, bool _seamlessLod, bool _forceParse)
{
    if (_pathes.empty())
    {
        kage::message(kage::error, "No path was provided!");
        return false;
    }

    // check the first file format
    const std::string& p = _pathes[0];
    const char* ext0 = getExtension(_pathes[0].c_str());

    if (strcmp(ext0, "gltf") == 0 || strcmp(ext0, "glb") == 0)
    {
        bool rcm = false;
        if (!_forceParse)
        {
            char path[256];
            strcpy(path, p.c_str());
            strcat(path, ".scene");
            rcm = loadSceneDump(_scene, path);
        }

        if (!rcm)
            rcm = loadGltfScene(_scene, p.c_str(), _buildMeshlets, _seamlessLod);
        return rcm;
    }

    if (strcmp(ext0, "obj") == 0)
    {
        return loadObjScene(_scene, _pathes, _buildMeshlets, _seamlessLod);
    }

    kage::message(kage::error, "Unsupported file format: %s", p.c_str());
    return false;
}