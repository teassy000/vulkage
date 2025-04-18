
#include "brixel_intg_kage.h"
#include "FidelityFX/host/ffx_brixelizer.h"
#include "brixel_structs.h"

#include <glm/gtx/quaternion.hpp> // for glm::toMat()
#include <glm/gtx/matrix_major_storage.hpp>  // for glm::rowMajor()
#include <vector>


void brxRegBuffers(kage::BufferHandle _vtx, uint32_t _vtxSz, uint32_t _vtxStride, kage::BufferHandle _idx, uint32_t _idxSz, uint32_t _idxStride)
{
    BrixelBufDescs descs[2];

    descs[0].buf = _vtx;
    descs[0].size = _vtxSz;
    descs[0].stride = _vtxStride;

    descs[1].buf = _idx;
    descs[1].size = _idxSz;
    descs[1].stride = _idxStride;

    const kage::Memory* bufDescMem = kage::alloc(sizeof(descs));
    std::memcpy(bufDescMem->data, descs, bufDescMem->size);

    kage::brx_regGeoBuffers(bufDescMem, _vtx, _idx);
}

void processBrxData(BrixelResources& _data, const Scene& _scene)
{
    size_t vtxCount = _scene.geometry.vertices.size();
    _data.vtxes.reserve(vtxCount);
    for (size_t ii = 0; ii < vtxCount; ++ii)
    {
        vec3 v;
        v.x = _scene.geometry.vertices[ii].vx;
        v.y = _scene.geometry.vertices[ii].vy;
        v.z = _scene.geometry.vertices[ii].vz;

        _data.vtxes.emplace_back(v);
    }
    _data.vtxes.shrink_to_fit();

    _data.meshes.reserve(_scene.geometry.meshes.size());

    size_t idxCount = _scene.geometry.indices.size();
    _data.idxes.reserve(idxCount);

    uint32_t idxOffset = 0;
    for (size_t ii = 0; ii < _scene.geometry.meshes.size(); ii++)
    {
        BRX_Mesh bm;
        const Mesh& mesh = _scene.geometry.meshes[ii];

        const MeshLod& lod0 = mesh.lods[0];
        const uint32_t* idxes = (const uint32_t*)_scene.geometry.indices.data() + lod0.indexOffset;

        _data.idxes.insert(_data.idxes.end(), idxes, idxes + lod0.indexCount);

        bm.vtx_count = mesh.vertexCount;
        bm.vtx_offset = mesh.vertexOffset;
        bm.idx_count = lod0.indexCount;
        bm.idx_offset = idxOffset;

        _data.meshes.emplace_back(bm);

        idxOffset = (uint32_t)_data.idxes.size();
    }

    _data.idxes.shrink_to_fit();
    _data.meshes.shrink_to_fit();

    // create the buffers
    const kage::Memory* memVtxBuf = kage::alloc((uint32_t)(_data.vtxes.size() * sizeof(vec3)));
    memcpy(memVtxBuf->data, _data.vtxes.data(), memVtxBuf->size);
    kage::BufferDesc vtxBufDesc;
    vtxBufDesc.size = memVtxBuf->size;
    vtxBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    vtxBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle brx_vtx_buf = kage::registBuffer("brx_vtx", vtxBufDesc, memVtxBuf);

    const kage::Memory* memIdxBuf = kage::alloc((uint32_t)(_data.idxes.size() * sizeof(uint32_t)));
    memcpy(memIdxBuf->data, _data.idxes.data(), memIdxBuf->size);
    kage::BufferDesc idxBufDesc;
    idxBufDesc.size = memIdxBuf->size;
    idxBufDesc.usage = kage::BufferUsageFlagBits::index | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::storage;
    idxBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle brx_idx_buf = kage::registBuffer("brx_idx", idxBufDesc, memIdxBuf);

    brxRegBuffers(brx_vtx_buf, memVtxBuf->size, sizeof(vec3), brx_idx_buf, memIdxBuf->size, sizeof(uint32_t));
}

mat4 modelMatrix(const vec3& _pos, const quat& _orit, const vec3& _scale)
{
    mat4 model = mat4(1.f);

    // m * t * r * s
    model = glm::translate(model, _pos);
    model = model * glm::toMat4(_orit);
    model = glm::scale(model, _scale);

    return  model;
}

void brxProcessScene(BrixelResources& _brx, const Scene& _scene, bool _seamless)
{
    assert(!_seamless); // not support seamless yet

    std::vector<FfxBrixelizerInstanceDescription> instDescs;
    instDescs.reserve(_scene.drawCount);

    for (uint32_t ii = 0; ii < _scene.drawCount; ++ii)
    {
        const MeshDraw& mdraw = _scene.meshDraws[ii];
        const Mesh& mesh = _scene.geometry.meshes[mdraw.meshIdx];
        const BRX_Mesh& bMesh = _brx.meshes[mdraw.meshIdx];

        vec3 center = mesh.center;
        float radius = mesh.radius;
        vec3 oriAabbMin = mesh.aabbMin;
        vec3 oriAabbMax = mesh.aabbMax;
        vec3 aabbExtent = oriAabbMax - oriAabbMin;

        const vec3 aabbCorners[8] = {
            oriAabbMin + vec3(0.f),
            oriAabbMin + vec3(aabbExtent.x, 0.f, 0.f),
            oriAabbMin + vec3(0.f, 0.f, aabbExtent.z),
            oriAabbMin + vec3(aabbExtent.x, 0.f, aabbExtent.z),
            oriAabbMin + vec3(0.f, aabbExtent.y, 0.f),
            oriAabbMin + vec3(aabbExtent.x, aabbExtent.y, 0.f),
            oriAabbMin + vec3(0.f, aabbExtent.y, aabbExtent.z),
            oriAabbMin + aabbExtent,
        };

        glm::mat4 transform = modelMatrix(mdraw.pos, mdraw.orit, vec3(mdraw.scale));

        vec3 minAABB = vec3(FLT_MAX);
        vec3 maxAABB = vec3(FLT_MIN);
        for (uint32_t jj = 0; jj < 8; ++jj) {
            vec3 cornerWorldPos = transform * vec4(aabbCorners[jj], 1.f);
            minAABB = glm::min(minAABB, cornerWorldPos);
            maxAABB = glm::max(maxAABB, cornerWorldPos);
        }


        FfxBrixelizerInstanceDescription instDesc = {};
        FfxBrixelizerInstanceID instId = FFX_BRIXELIZER_INVALID_ID;

        instDesc.maxCascade = kage::k_brixelizerCascadeCount;
        for (uint32_t jj = 0; jj < 3; ++jj) {
            instDesc.aabb.min[jj] = minAABB[jj];
            instDesc.aabb.max[jj] = maxAABB[jj];
        }
        
        // store the transformation matrix in row major
        for (uint32_t jj = 0; jj < 3; ++jj) {
            for (uint32_t kk = 0; kk < 4; ++kk) {
                instDesc.transform[jj * 4 + kk] = transform[kk][jj];
            }
        }

        instDesc.indexFormat = FFX_INDEX_TYPE_UINT32;
        instDesc.indexBuffer = 0; // reserve and set in the backend part
        instDesc.indexBufferOffset = bMesh.idx_offset * sizeof(uint32_t);
        instDesc.triangleCount = bMesh.idx_count / 3;
        instDesc.vertexBuffer = 0; // reserve and set in the backend part
        instDesc.vertexStride = sizeof(vec3);
        instDesc.vertexBufferOffset = bMesh.vtx_offset * sizeof(vec3);
        instDesc.vertexCount = bMesh.vtx_count;
        instDesc.vertexFormat = FFX_SURFACE_FORMAT_R32G32B32_FLOAT;
        instDesc.flags = FFX_BRIXELIZER_INSTANCE_FLAG_NONE; //static

        instDesc.outInstanceID = nullptr;

        instDescs.emplace_back(instDesc);
    }

    const kage::Memory* descMem = kage::alloc((uint32_t)(instDescs.size() * sizeof(FfxBrixelizerInstanceDescription)));
    std::memcpy(descMem->data, instDescs.data(), descMem->size);

    kage::brx_setGeoInstances(descMem);
}

void brxCreateResources(BrixelResources& _data)
{
    std::vector<kage::UnifiedResHandle> handles;
    handles.reserve(4 + FFX_BRIXELIZER_MAX_CASCADES * 2);

    // debug dest image
    {
        kage::ImageDesc debugDestDesc = {};
        debugDestDesc.width = 2560;
        debugDestDesc.height = 1440;
        debugDestDesc.depth = 1;
        debugDestDesc.numLayers = 1;
        debugDestDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
        debugDestDesc.usage = kage::ImageUsageFlagBits::storage | kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled;
        debugDestDesc.layout = kage::ImageLayout::general;
        debugDestDesc.viewType = kage::ImageViewType::type_2d;
        _data.debugDestImg = kage::registTexture("brxl_debug_dest", debugDestDesc);
    }

    // scratch buf
    {
        kage::BufferDesc scratchBufDesc = {};
        scratchBufDesc.size = 128 * 1024 * 1024;
        scratchBufDesc.fillVal = 0;
        scratchBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        scratchBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::transfer_src;
        _data.scratchBuf = kage::registBuffer("brxl_scratch_buf", scratchBufDesc);
    }

    // sdf atlas
    {
        kage::ImageDesc sdfAtlasDesc = {};
        sdfAtlasDesc.width = FFX_BRIXELIZER_STATIC_CONFIG_SDF_ATLAS_SIZE;
        sdfAtlasDesc.height = FFX_BRIXELIZER_STATIC_CONFIG_SDF_ATLAS_SIZE;
        sdfAtlasDesc.depth = FFX_BRIXELIZER_STATIC_CONFIG_SDF_ATLAS_SIZE;
        sdfAtlasDesc.numLayers = 1;
        sdfAtlasDesc.format = kage::ResourceFormat::r8_unorm;
        sdfAtlasDesc.usage = kage::ImageUsageFlagBits::storage | kage::ImageUsageFlagBits::sampled;
        sdfAtlasDesc.layout = kage::ImageLayout::general;
        sdfAtlasDesc.type = kage::ImageType::type_3d;
        sdfAtlasDesc.viewType = kage::ImageViewType::type_3d;

        _data.userReses.sdfAtlas = kage::registTexture("brxl_sdf_atlas", sdfAtlasDesc);
    }

    // brick aabbs
    {
        kage::BufferDesc brickAABBDesc = {};
        brickAABBDesc.size = FFX_BRIXELIZER_BRICK_AABBS_SIZE;
        brickAABBDesc.fillVal = 0;
        brickAABBDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        brickAABBDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::transfer_src;

        _data.userReses.brickAABB = kage::registBuffer("brxl_brick_aabbs", brickAABBDesc);
    }

    // cascade aabb trees
    {
        kage::BufferDesc cascadeAABBTreeDesc = {};
        cascadeAABBTreeDesc.size = FFX_BRIXELIZER_CASCADE_AABB_TREE_SIZE;
        cascadeAABBTreeDesc.fillVal = 0;
        cascadeAABBTreeDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        cascadeAABBTreeDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::transfer_src;

        for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
        {
            std::string name = "brxl_cascade_aabbs_";
            name += std::to_string(ii);
            _data.userReses.cascadeAABBTrees[ii] = kage::registBuffer(name.c_str(), cascadeAABBTreeDesc);
        }
    }

    // cascade brick maps
    {
        kage::BufferDesc cascadeBrickMapDesc = {};
        cascadeBrickMapDesc.size = FFX_BRIXELIZER_CASCADE_BRICK_MAP_SIZE;
        cascadeBrickMapDesc.fillVal = 0;
        cascadeBrickMapDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        cascadeBrickMapDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::transfer_src;

        for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
        {
            std::string name = "brxl_cascade_brick_map_";
            name += std::to_string(ii);
            _data.userReses.cascadeBrickMaps[ii] = kage::registBuffer(name.c_str(), cascadeBrickMapDesc);
        }
    }

    // reserved cascade infos
    {
        kage::BufferDesc cascadeInfoDesc = {};
        cascadeInfoDesc.size = sizeof(FfxBrixelizerCascadeInfo) * FFX_BRIXELIZER_MAX_CASCADES;
        cascadeInfoDesc.fillVal = 0;
        cascadeInfoDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        cascadeInfoDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::transfer_src;
        _data.userReses.cascadeInfos = kage::registBuffer("brxl_cascade_infos", cascadeInfoDesc);
    }

    handles.emplace_back( _data.debugDestImg );
    handles.emplace_back( _data.scratchBuf );
    handles.emplace_back( _data.userReses.sdfAtlas );
    handles.emplace_back( _data.userReses.brickAABB );
    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        handles.emplace_back( _data.userReses.cascadeAABBTrees[ii] );
    }
    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        handles.emplace_back(_data.userReses.cascadeBrickMaps[ii]);
    }
    handles.emplace_back(_data.userReses.cascadeInfos);

    const kage::Memory* bufMem = kage::alloc(uint32_t(handles.size() * sizeof(kage::UnifiedResHandle)));
    std::memcpy(bufMem->data, handles.data(), bufMem->size);

    kage::brx_setUserResources(bufMem);
}

void brxInit(BrixelResources& _bxl, const BrixelInitDesc& _init, const Scene& _scene)
{
    processBrxData(_bxl, _scene);
    brxProcessScene(_bxl, _scene, _init.seamless);
    brxCreateResources(_bxl);
}

void brxUpdate(BrixelResources& _bxl, const BrixelData& _data)
{
    mat4 invProj = glm::inverse(_data.projMat);
    mat4 invView = glm::inverse(_data.viewMat);
    
    BrixelDebugDescs desc;
    for (uint32_t ii = 0; ii < 4; ++ii) {
        for (uint32_t jj = 0; jj < 4; ++jj) {
            desc.proj[ii * 4 + jj] = invProj[ii][jj];
            desc.view[ii * 4 + jj] = invView[ii][jj];
        }
    }

    vec3 cpos = _data.camPos;
    std::memcpy(desc.camPos, &cpos[0], sizeof(float) * 3);

    desc.start_cas = std::min(_data.startCas, _data.endCas);
    desc.end_cas = std::max(std::max(_data.startCas, _data.endCas), kage::k_brixelizerCascadeCount - 1);
    desc.sdf_eps = _data.sdfEps;
    desc.tmin = _data.tmin;
    desc.tmax = _data.tmax;
    desc.debugType = (BrixelDebugType)_data.debugType;
    desc.followCam = _data.followCam;

    const kage::Memory* mem = kage::alloc(sizeof(BrixelDebugDescs));

    std::memcpy(mem->data, &desc, sizeof(BrixelDebugDescs));

    kage::brx_setDebugInfos(mem);
}

