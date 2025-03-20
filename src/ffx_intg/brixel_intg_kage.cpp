
#include "brixel_intg_kage.h"
#include "FidelityFX/host/ffx_brixelizer.h"
#include "brixel_structs.h"

#include <glm/gtx/quaternion.hpp> // for glm::toMat()
#include <glm/gtx/matrix_major_storage.hpp>  // for glm::rowMajor()
#include <vector>


constexpr uint32_t c_brixelizerCascadeCount = (FFX_BRIXELIZER_MAX_CASCADES / 3);

mat4 modelMatrix(const vec3& _pos, const quat& _orit, const vec3& _scale)
{
    mat4 model = mat4(1.f);

    // m * t * r * s
    model = glm::translate(model, _pos);
    model = model * glm::toMat4(_orit);
    model = glm::scale(model, _scale);

    return  model;
}

void brxProcessScene(const Scene& _scene, bool _seamless)
{
    assert(!_seamless); // not support seamless yet

    std::vector<FfxBrixelizerInstanceDescription> instDescs;
    instDescs.reserve(_scene.drawCount);

    for (uint32_t ii = 0; ii < _scene.drawCount; ++ii)
    {
        const MeshDraw& mdraw = _scene.meshDraws[ii];
        const Mesh& mesh = _scene.geometry.meshes[mdraw.meshIdx];

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

        instDesc.maxCascade = c_brixelizerCascadeCount;
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
        instDesc.indexBufferOffset = mesh.lods[0].indexOffset;
        instDesc.triangleCount = mesh.lods[0].indexCount / 3;
        instDesc.vertexBuffer = 0; // reserve and set in the backend part
        instDesc.vertexStride = sizeof(Vertex);
        instDesc.vertexBufferOffset = mesh.vertexOffset;
        instDesc.vertexCount = mesh.vertexCount;
        instDesc.vertexFormat = FFX_SURFACE_FORMAT_R32G32B32_FLOAT;
        instDesc.flags = FFX_BRIXELIZER_INSTANCE_FLAG_NONE; //static

        instDesc.outInstanceID = nullptr;

        instDescs.emplace_back(instDesc);
    }

    const kage::Memory* descMem = kage::alloc((uint32_t)(instDescs.size() * sizeof(FfxBrixelizerInstanceDescription)));
    std::memcpy(descMem->data, instDescs.data(), descMem->size);

    kage::brx_setGeoInstances(descMem);
}

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

    kage::brx_regGeoBuffers(bufDescMem);
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

        _data.sdfAtlas = kage::registTexture("brxl_sdf_atlas", sdfAtlasDesc);
    }

    // brick aabbs
    {
        kage::BufferDesc brickAABBDesc = {};
        brickAABBDesc.size = FFX_BRIXELIZER_BRICK_AABBS_SIZE;
        brickAABBDesc.fillVal = 0;
        brickAABBDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        brickAABBDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::transfer_src;

        _data.brickAABB = kage::registBuffer("brxl_brick_aabbs", brickAABBDesc);
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
            _data.cascadeAABBTrees[ii] = kage::registBuffer(name.c_str(), cascadeAABBTreeDesc);
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
            _data.cascadeBrickMaps[ii] = kage::registBuffer(name.c_str(), cascadeBrickMapDesc);
        }
    }

    handles.emplace_back( _data.debugDestImg );
    handles.emplace_back( _data.scratchBuf );
    handles.emplace_back( _data.sdfAtlas );
    handles.emplace_back( _data.brickAABB );
    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        handles.emplace_back( _data.cascadeAABBTrees[ii] );
    }
    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        handles.emplace_back(_data.cascadeBrickMaps[ii]);
    }

    const kage::Memory* bufMem = kage::alloc(uint32_t(handles.size() * sizeof(kage::UnifiedResHandle)));
    std::memcpy(bufMem->data, handles.data(), bufMem->size);

    kage::brx_setUserResources(bufMem);
}

void brxInit(BrixelResources& _bxl, const BrixelInitDesc& _init, const Scene& _scene)
{
    brxRegBuffers(_init.vtxBuf, _init.vtxSz, _init.vtxStride, _init.idxBuf, _init.idxSz, _init.idxStride);
    brxProcessScene(_scene, _init.seamless);
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

    desc.start_cas = 0;
    desc.end_cas = c_brixelizerCascadeCount - 1;
    desc.sdf_eps = 0.01f;
    desc.tmin = 0.0f;
    desc.tmax = 1000.0f;
    desc.debugType = (BrixelDebugType)_data.debugType;

    const kage::Memory* mem = kage::alloc(sizeof(BrixelDebugDescs));

    std::memcpy(mem->data, &desc, sizeof(BrixelDebugDescs));

    kage::brx_setDebugInfos(mem);
}

