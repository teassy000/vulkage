#include "brixelizer_intg.h"
#include "debug.h"


#include "scene.h"
#include "mesh.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_major_storage.hpp>


constexpr uint32_t c_brixelizerCascadeCount = (FFX_BRIXELIZER_MAX_CASCADES / 3);

struct BrixelizerConfig
{
    float meshUnitSize{ .2f };
    float cascadeSizeRatio{ 2.f };
};

const static BrixelizerConfig s_conf{};


void createBrixelizerCtx(FFX_Brixelizer_Impl& _ffx)
{
    _ffx.initDesc.sdfCenter[0] = 0.f;
    _ffx.initDesc.sdfCenter[1] = 0.f;
    _ffx.initDesc.sdfCenter[2] = 0.f;
    _ffx.initDesc.numCascades = c_brixelizerCascadeCount;
    _ffx.initDesc.flags = FFX_BRIXELIZER_CONTEXT_FLAG_ALL_DEBUG;

    float voxSize = s_conf.meshUnitSize;
    for (uint32_t ii = 0; ii < _ffx.initDesc.numCascades; ++ii)
    {
        FfxBrixelizerCascadeDescription& refDesc = _ffx.initDesc.cascadeDescs[ii];
        
        refDesc.flags = (FfxBrixelizerCascadeFlag)(FFX_BRIXELIZER_CASCADE_STATIC | FFX_BRIXELIZER_CASCADE_DYNAMIC);
        refDesc.voxelSize = voxSize;

        voxSize *= s_conf.cascadeSizeRatio;
    }

    _ffx.initDesc.backendInterface;

    FfxErrorCode err = ffxBrixelizerContextCreate(&_ffx.initDesc, &_ffx.context);
    if (err != FFX_OK)
        kage::message(kage::error, "failed to create brixelizer context with error code %d.", err);
}

// grab from shader/math.h
vec3 rotateQuat(const vec3 _v, const quat _q) 
{
    return _v + 2.f * glm::cross(vec3(_q.x, _q.y, _q.z), glm::cross(vec3(_q.x, _q.y, _q.z), _v) + _q.w * _v);
}

mat4 modelMatrix(const vec3& _pos, const quat& _orit, const vec3& _scale, bool _rowMajor = false)
{
    mat4 model = mat4(1.f);

    model = glm::scale(model, _scale);
    model = glm::toMat4(_orit) * model;
    model = glm::translate(model, _pos);

    if (_rowMajor)
        model = glm::rowMajor4(model);

    return model;
}

void createFfxKageResouces(FFX_Brixelizer_Impl& _ffx)
{
    {
        kage::PassDesc pd{};
        pd.queue = kage::PassExeQueue::extern_abstract;
        
        _ffx.pass = kage::registPass("brxl", pd);
    }

    // scratch buffer
    {
        kage::BufferDesc scratchDesc = {};
        scratchDesc.size = _ffx.gpuScratchedBufferSize;
        scratchDesc.fillVal = 0;
        scratchDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        scratchDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::transfer_src;
        _ffx.scratchBuf = kage::registBuffer("brxl_scratch", scratchDesc);
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

        _ffx.sdfAtlas = kage::registTexture("brxl_sdf_atlas", sdfAtlasDesc);
    }

    // brick aabbs
    {
        kage::BufferDesc brickAABBDesc = {};
        brickAABBDesc.size = FFX_BRIXELIZER_BRICK_AABBS_SIZE;
        brickAABBDesc.fillVal = 0;
        brickAABBDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        brickAABBDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::transfer_src;

        _ffx.brickAABB = kage::registBuffer("brxl_brick_aabbs", brickAABBDesc);
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
            _ffx.cascadeAABBTrees[ii] = kage::registBuffer("brxl_cascade_aabbs", cascadeAABBTreeDesc);
        }
    }

    // cascade brick maps
    {
        kage::BufferDesc cascadeBrickMapDesc = {};
        cascadeBrickMapDesc.size = FFX_BRIXELIZER_MAX_CASCADES * FFX_BRIXELIZER_CASCADE_BRICK_MAP_SIZE;
        cascadeBrickMapDesc.fillVal = 0;
        cascadeBrickMapDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        cascadeBrickMapDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::transfer_src;

        for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
        {
            _ffx.cascadeBrickMaps[ii] = kage::registBuffer("brxl_cascade_brick_map", cascadeBrickMapDesc);
        }
    }

    kage::BufferHandle scratchAlias = kage::alias(_ffx.scratchBuf);
    kage::bindBuffer(_ffx.pass, _ffx.scratchBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , scratchAlias
    );


    _ffx.sdfAtlasOutAlias = kage::alias(_ffx.sdfAtlas);
    kage::bindImage(_ffx.pass, _ffx.sdfAtlas
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , _ffx.sdfAtlasOutAlias
    );

    kage::BufferHandle brickAABBAlias = kage::alias(_ffx.brickAABB);
    kage::bindBuffer(_ffx.pass, _ffx.brickAABB
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , brickAABBAlias
    );

    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        kage::BufferHandle cascadeAABBAlias = kage::alias(_ffx.cascadeAABBTrees[ii]);
        kage::bindBuffer(_ffx.pass, _ffx.cascadeAABBTrees[ii]
            , kage::PipelineStageFlagBits::compute_shader
                , kage::AccessFlagBits::shader_write
                , cascadeAABBAlias
                );
        kage::BufferHandle cascadeBrickMapAlias = kage::alias(_ffx.cascadeBrickMaps[ii]);
        kage::bindBuffer(_ffx.pass, _ffx.cascadeBrickMaps[ii]
            , kage::PipelineStageFlagBits::compute_shader
                , kage::AccessFlagBits::shader_write
                , cascadeBrickMapAlias
                );
    }


}

void registerBrixelizerBuffers(FFX_Brixelizer_Impl& _ffx, const Scene& _scene, const kage::BufferHandle _vtxBuf, const kage::BufferHandle _idxBuf)
{
    FfxBrixelizerBufferDescription bufferDescs[2] = {};
    uint32_t bufferIdxes[2] = {};

    FfxResource vtxRes;
    vtxRes.resource = kage::getRHIResource(_vtxBuf);
    vtxRes.description.type = FFX_RESOURCE_TYPE_BUFFER;
    vtxRes.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
    vtxRes.description.size = uint32_t(_scene.geometry.vertices.size() * sizeof(Vertex));
    vtxRes.description.stride = sizeof(Vertex);
    vtxRes.description.alignment = 0;
    vtxRes.description.mipCount = 0;
    vtxRes.description.flags = FFX_RESOURCE_FLAGS_NONE;
    vtxRes.description.usage = FFX_RESOURCE_USAGE_READ_ONLY;
    vtxRes.state = FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ;
    std::string nameStr = "brixelizer vertex buffer";
    std::copy(nameStr.begin(), nameStr.end(), vtxRes.name);

    FfxResource idxRes;
    idxRes.resource = kage::getRHIResource(_idxBuf);
    idxRes.description.type = FFX_RESOURCE_TYPE_BUFFER;
    idxRes.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
    idxRes.description.size = uint32_t(_scene.geometry.indices.size() * sizeof(uint32_t));
    idxRes.description.stride = sizeof(uint32_t);
    idxRes.description.alignment = 0;
    idxRes.description.mipCount = 0;
    idxRes.description.flags = FFX_RESOURCE_FLAGS_NONE;
    idxRes.description.usage = FFX_RESOURCE_USAGE_READ_ONLY;
    idxRes.state = FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ;
    nameStr = "brixelizer index buffer";
    std::copy(nameStr.begin(), nameStr.end(), idxRes.name);

    bufferDescs[0].buffer = vtxRes;
    bufferDescs[0].outIndex = &(bufferIdxes[0]);
    bufferDescs[1].buffer = idxRes;
    bufferDescs[1].outIndex = &(bufferIdxes[1]);

    FfxErrorCode err = ffxBrixelizerRegisterBuffers(&_ffx.context, bufferDescs, COUNTOF(bufferDescs));

    if (err != FFX_OK)
        kage::message(kage::error, "failed to register brixelizer buffer with error code %d.", err);
}

void createBrixelizerInstances(FFX_Brixelizer_Impl& _ffx, const Scene& _scene, bool _seamless = false)
{
    assert(!_seamless); // not support seamless yet

    std::vector<FfxBrixelizerInstanceDescription> instDescs;
    std::vector<FfxBrixelizerInstanceID> instIds;
    instDescs.reserve(_scene.drawCount);
    instDescs.reserve(_scene.drawCount);


    for (uint32_t ii = 0; ii < _scene.drawCount; ++ ii)
    {
        const MeshDraw& mdraw = _scene.meshDraws[ii];
        const Mesh& mesh = _scene.geometry.meshes[mdraw.meshIdx];

        vec3 center = mesh.center;
        float radius = mesh.radius;
        vec3 oriAabbMin = center - radius;
        vec3 oriAabbMax = center + radius;
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

        vec3 minAABB = vec3(FLT_MAX);
        vec3 maxAABB = vec3(FLT_MIN);
        for (uint32_t jj = 0; jj < 8; ++jj) {
            vec3 worldPos = rotateQuat(aabbCorners[jj], mdraw.orit)* mdraw.scale + mdraw.pos;
            minAABB = glm::min(minAABB, worldPos);
            maxAABB = glm::max(maxAABB, worldPos);
        }

        glm::mat4 transform = modelMatrix(mdraw.pos, mdraw.orit, vec3(mdraw.scale), true);

        FfxBrixelizerInstanceDescription instDesc = {};
        FfxBrixelizerInstanceID instId = FFX_BRIXELIZER_INVALID_ID;

        instDesc.maxCascade = c_brixelizerCascadeCount;
        for (uint32_t jj = 0; jj < 3; ++jj) {
            instDesc.aabb.min[jj] = minAABB[jj];
            instDesc.aabb.max[jj] = maxAABB[jj];
        }

        for (uint32_t jj = 0; jj < 3; ++jj)
            for (uint32_t kk = 0; kk < 4; ++kk)
                instDesc.transform[jj * 4 + kk] = transform[jj][kk];

        instDesc.indexFormat = FFX_INDEX_TYPE_UINT32;
        instDesc.indexBuffer = 0; // now we only have one buffer
        instDesc.indexBufferOffset = mesh.lods[0].indexOffset;
        instDesc.triangleCount = mesh.lods[0].indexCount / 3;
        instDesc.vertexBuffer = 0; // now we only have one buffer
        instDesc.vertexStride = sizeof(Vertex);
        instDesc.vertexBufferOffset = mesh.vertexOffset;
        instDesc.vertexCount = mesh.vertexCount;
        instDesc.vertexFormat = FFX_SURFACE_FORMAT_R32G32B32_FLOAT;
        instDesc.flags = FFX_BRIXELIZER_INSTANCE_FLAG_NONE; //static

        instIds.emplace_back(instId);

        instDesc.outInstanceID = &(instIds.back());

        instDescs.emplace_back(instDesc);
    }

    FfxErrorCode err = ffxBrixelizerCreateInstances(&_ffx.context, instDescs.data(), (uint32_t)instDescs.size());

    if (err != FFX_OK)
        kage::message(kage::error, "failed to create brixelizer instance with error code %d.", err);
}


void postInitBrixelizerImpl(FFX_Brixelizer_Impl& _ffx, const Scene& _scene)
{
    if(!_ffx.postInitilized && kage::isBackendReady())
    {
        registerBrixelizerBuffers(_ffx, _scene, _ffx.vtxBuf, _ffx.idxBuf);
        createBrixelizerInstances(_ffx, _scene);

        _ffx.postInitilized = true;
    }
}

using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;
using LoadOp = kage::AttachmentLoadOp;
using StoreOp = kage::AttachmentStoreOp;

void recBrixelizer(FFX_Brixelizer_Impl& _ffx, FfxBrixelizerUpdateDescription& _updateDesc)
{
    FfxResource scratchRes{};
    scratchRes.resource = kage::getRHIResource(_ffx.scratchBuf);
    scratchRes.description.type = FFX_RESOURCE_TYPE_BUFFER;
    scratchRes.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
    scratchRes.description.size = FFX_BRIXELIZER_CASCADE_BRICK_MAP_SIZE;
    scratchRes.description.stride = FFX_BRIXELIZER_CASCADE_BRICK_MAP_STRIDE;
    scratchRes.description.alignment = 0;
    scratchRes.description.mipCount = 0;
    scratchRes.description.flags = FFX_RESOURCE_FLAGS_NONE;
    scratchRes.description.usage = FFX_RESOURCE_USAGE_UAV;
    scratchRes.state = FFX_RESOURCE_STATE_UNORDERED_ACCESS;
    std::string nameStr = "brixelizer scratch buffer";
    std::copy(nameStr.begin(), nameStr.end(), scratchRes.name);

    const kage::Memory* mem = kage::alloc(sizeof(scratchRes));
    std::memcpy(mem->data, &scratchRes, mem->size);

    kage::startRec(_ffx.pass);

    std::vector<kage::Binding> binds;
    binds.reserve(3 + FFX_BRIXELIZER_MAX_CASCADES * 2);
    binds.emplace_back(kage::Binding{ _ffx.sdfAtlas, 0, Stage::compute_shader });
    binds.emplace_back(kage::Binding{ _ffx.brickAABB, Access::read_write, Stage::compute_shader });
    binds.emplace_back(kage::Binding{ _ffx.scratchBuf, Access::read_write, Stage::compute_shader });

    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        binds.emplace_back(kage::Binding{ _ffx.cascadeAABBTrees[ii], Access::read_write, Stage::compute_shader });
        binds.emplace_back(kage::Binding{ _ffx.cascadeBrickMaps[ii], Access::read_write, Stage::compute_shader });
    }

    kage::pushBindings(binds.data(), (uint16_t)binds.size());

    kage::updateBrixelizer((void*)&_ffx.context, &_updateDesc, mem);

    kage::endRec();
}

void updateBrixelizer(FFX_Brixelizer_Impl& _ffx)
{
    FfxBrixelizerUpdateDescription updateDesc = {};
    std::string nameStr;

    // sdf atlas
    updateDesc.resources.sdfAtlas.resource = kage::getRHIResource(_ffx.sdfAtlas);
    updateDesc.resources.sdfAtlas.description.type = FFX_RESOURCE_TYPE_TEXTURE3D;
    updateDesc.resources.sdfAtlas.description.format = FFX_SURFACE_FORMAT_R8_UINT;
    updateDesc.resources.sdfAtlas.description.size = FFX_BRIXELIZER_STATIC_CONFIG_SDF_ATLAS_SIZE * FFX_BRIXELIZER_STATIC_CONFIG_SDF_ATLAS_SIZE * FFX_BRIXELIZER_STATIC_CONFIG_SDF_ATLAS_SIZE;
    updateDesc.resources.sdfAtlas.description.stride = 1u;
    updateDesc.resources.sdfAtlas.description.alignment = 0;
    updateDesc.resources.sdfAtlas.description.mipCount = 0;
    updateDesc.resources.sdfAtlas.description.flags = FFX_RESOURCE_FLAGS_NONE;
    updateDesc.resources.sdfAtlas.description.usage = FFX_RESOURCE_USAGE_READ_ONLY;
    updateDesc.resources.sdfAtlas.state = FFX_RESOURCE_STATE_COMMON;
    nameStr = "brixelizer sdf atlas";
    std::copy(nameStr.begin(), nameStr.end(), updateDesc.resources.sdfAtlas.name);

    // brick aabbs
    updateDesc.resources.brickAABBs.resource = kage::getRHIResource(_ffx.brickAABB);
    updateDesc.resources.brickAABBs.description.type = FFX_RESOURCE_TYPE_BUFFER;
    updateDesc.resources.brickAABBs.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
    updateDesc.resources.brickAABBs.description.size = FFX_BRIXELIZER_BRICK_AABBS_SIZE;
    updateDesc.resources.brickAABBs.description.stride = FFX_BRIXELIZER_BRICK_AABBS_STRIDE;
    updateDesc.resources.brickAABBs.description.alignment = 0;
    updateDesc.resources.brickAABBs.description.mipCount = 0;
    updateDesc.resources.brickAABBs.description.flags = FFX_RESOURCE_FLAGS_NONE;
    updateDesc.resources.brickAABBs.description.usage = FFX_RESOURCE_USAGE_UAV;
    updateDesc.resources.brickAABBs.state = FFX_RESOURCE_STATE_COMMON;
    nameStr = "brixelizer brick aabbs";
    std::copy(nameStr.begin(), nameStr.end(), updateDesc.resources.brickAABBs.name);

    // cascade aabb tree
    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        FfxBrixelizerCascadeResources& refRes = updateDesc.resources.cascadeResources[ii];

        refRes.aabbTree.resource = kage::getRHIResource(_ffx.cascadeAABBTrees[ii]);
        refRes.aabbTree.description.type = FFX_RESOURCE_TYPE_BUFFER;
        refRes.aabbTree.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        refRes.aabbTree.description.size = FFX_BRIXELIZER_CASCADE_AABB_TREE_SIZE;
        refRes.aabbTree.description.stride = FFX_BRIXELIZER_CASCADE_AABB_TREE_STRIDE;
        refRes.aabbTree.description.alignment = 0;
        refRes.aabbTree.description.mipCount = 0;
        refRes.aabbTree.description.flags = FFX_RESOURCE_FLAGS_NONE;
        refRes.aabbTree.description.usage = FFX_RESOURCE_USAGE_UAV;
        refRes.aabbTree.state = FFX_RESOURCE_STATE_COMMON;
        nameStr = "brixelizer cascade aabb:" + std::to_string(ii);
        std::copy(nameStr.begin(), nameStr.end(), refRes.aabbTree.name);

        refRes.brickMap.resource = kage::getRHIResource(_ffx.cascadeAABBTrees[ii]);
        refRes.brickMap.description.type = FFX_RESOURCE_TYPE_BUFFER;
        refRes.brickMap.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        refRes.brickMap.description.size = FFX_BRIXELIZER_CASCADE_BRICK_MAP_SIZE;
        refRes.brickMap.description.stride = FFX_BRIXELIZER_CASCADE_BRICK_MAP_STRIDE;
        refRes.brickMap.description.alignment = 0;
        refRes.brickMap.description.mipCount = 0;
        refRes.brickMap.description.flags = FFX_RESOURCE_FLAGS_NONE;
        refRes.brickMap.description.usage = FFX_RESOURCE_USAGE_UAV;
        refRes.brickMap.state = FFX_RESOURCE_STATE_COMMON;
        nameStr = "brixelizer cascade brick map:" + std::to_string(ii);
        std::copy(nameStr.begin(), nameStr.end(), refRes.brickMap.name);
    }

    FfxBrixelizerStats stats = {};
    size_t scratchBufferSize = 0;

    updateDesc.frameIndex = _ffx.frameIdx;
    updateDesc.sdfCenter[0] = _ffx.initDesc.sdfCenter[0];
    updateDesc.sdfCenter[1] = _ffx.initDesc.sdfCenter[1];
    updateDesc.sdfCenter[2] = _ffx.initDesc.sdfCenter[2];
    updateDesc.populateDebugAABBsFlags = FFX_BRIXELIZER_POPULATE_AABBS_NONE;
    updateDesc.debugVisualizationDesc = nullptr;
    updateDesc.maxReferences = 32 * (1 << 20);
    updateDesc.maxBricksPerBake = 1 << 14;
    updateDesc.triangleSwapSize = 300 * (1 << 20);
    updateDesc.outStats = &stats;
    updateDesc.outScratchBufferSize = &scratchBufferSize;
    
    // barriers pre
    
    FfxErrorCode err = ffxBrixelizerBakeUpdate(&_ffx.context, &updateDesc, &_ffx.bakedUpdateDesc);
    if (err != FFX_OK) {
        kage::message(kage::error, "failed to bake update with error code %d.", err);
    }

    if(scratchBufferSize > _ffx.gpuScratchedBufferSize)
    {
        kage::updateBuffer(_ffx.scratchBuf, nullptr, 0u, (uint32_t)scratchBufferSize);
        _ffx.gpuScratchedBufferSize = scratchBufferSize;
    }


    recBrixelizer(_ffx, updateDesc);

    _ffx.frameIdx++;
}


void initBrixelizerImpl(FFX_Brixelizer_Impl& _ffx, const kage::BufferHandle _vtxBuf, const kage::BufferHandle _idxBuf)
{
    kage::getFFXInterface(static_cast<void*>(&_ffx.initDesc.backendInterface));

    createBrixelizerCtx(_ffx);

    createFfxKageResouces(_ffx);

    _ffx.vtxBuf = _vtxBuf;
    _ffx.idxBuf = _idxBuf;

    kage::bindBuffer(_ffx.pass, _ffx.vtxBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(_ffx.pass, _ffx.idxBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );
}

void destroyBrixelizerImpl(FFX_Brixelizer_Impl& _ffx)
{
    ffxBrixelizerContextDestroy(&_ffx.context);
}

void updateBrixellizerImpl(const FFX_Brixelizer_Impl& _ffx, const Scene& _scene)
{
    postInitBrixelizerImpl(const_cast<FFX_Brixelizer_Impl&>(_ffx), _scene);

    if (_ffx.postInitilized)
    {
        updateBrixelizer(const_cast<FFX_Brixelizer_Impl&>(_ffx));
    }
    else
    {
        // workaround for no record at first frame 
        kage::startRec(_ffx.pass);
        kage::endRec();
    }
}
