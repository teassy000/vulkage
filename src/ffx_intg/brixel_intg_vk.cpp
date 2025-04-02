#include "brixel_intg_vk.h"
#include "debug.h"
#include "kage_math.h"
#include "brixel_structs.h"

#include "common.h" // for stl

#include "FidelityFX/host/backends/vk/ffx_vk.h" // for init_ffx

#include "rhi_context_vk.h" // for s_renderVK

namespace kage { namespace vk {

extern RHIContext_vk* s_renderVK; // accessing Vk context

namespace brx {

#define FFX_CHECK(err) if (err != FFX_OK) \
    { kage::message(kage::error, "ffx check failed with error code %d.", err); }

constexpr uint32_t c_brixelizerCascadeCount = (FFX_BRIXELIZER_MAX_CASCADES / 3);

struct BrixelizerConfig
{
    float meshUnitSize{ .1f };
    float cascadeSizeRatio{ 2.f };
};

const static BrixelizerConfig s_conf{};

void createCtx(FFXBrixelizer_vk& _brx)
{
    _brx.initDesc.sdfCenter[0] = 0.f;
    _brx.initDesc.sdfCenter[1] = 0.f;
    _brx.initDesc.sdfCenter[2] = 0.f;
    _brx.initDesc.numCascades = c_brixelizerCascadeCount;
    _brx.initDesc.flags = FFX_BRIXELIZER_CONTEXT_FLAG_ALL_DEBUG;

    float voxSize = s_conf.meshUnitSize;
    for (uint32_t ii = 0; ii < _brx.initDesc.numCascades; ++ii)
    {
        FfxBrixelizerCascadeDescription& refDesc = _brx.initDesc.cascadeDescs[ii];

        refDesc.flags = (FfxBrixelizerCascadeFlag)(FFX_BRIXELIZER_CASCADE_STATIC | FFX_BRIXELIZER_CASCADE_DYNAMIC);
        refDesc.voxelSize = voxSize;

        voxSize *= s_conf.cascadeSizeRatio;
    }

    _brx.initDesc.backendInterface = _brx.ffxCtx.interface;

    FFX_CHECK(ffxBrixelizerContextCreate(&_brx.initDesc, &_brx.context));
}

void setGeoInstances(FFXBrixelizer_vk& _brx, const Memory* _descs)
{
    if (nullptr == _descs) {
        message(error, "invalid _desc ptr!");
        return;
    }

    _brx.mem_geoInstDescs = _descs;
}

void setGeoBuffers(FFXBrixelizer_vk& _brx, const Memory* _buf)
{
    if (nullptr == _buf) {
        message(error, "invalid _buffers ptr!");
        return;
    }

    _brx.mem_geoBuf = _buf;
}

void setUserResources(FFXBrixelizer_vk& _brx, const Memory* _reses)
{
    if (nullptr == _reses) {
        message(error, "invalid _reses ptr!");
        return;
    }

    _brx.mem_userRes = _reses;
}

void setDebugInfos(FFXBrixelizer_vk& _brx, const Memory* _debug)
{
    if (nullptr == _debug) {
        message(error, "invalid debug infos!");
        return;
    }
    _brx.mem_debugInfos = _debug;
}

void parseGeoInst_n_submit(FFXBrixelizer_vk& _brx)
{
    if (nullptr == _brx.mem_geoInstDescs) {
        message(error, "invalid geo instance descs!");
        return;
    }
    
    uint32_t instCount = _brx.mem_geoInstDescs->size / sizeof(FfxBrixelizerInstanceDescription);

    if (instCount == 0) {
        message(error, "invalid geo instance count!");
        return;
    }

    stl::vector<FfxBrixelizerInstanceDescription>& descsRef = _brx.geoInstDescs;
    stl::vector<FfxBrixelizerInstanceID>& idsRef = _brx.geoInstIds; 

    FfxBrixelizerInstanceDescription* desc = (FfxBrixelizerInstanceDescription*)_brx.mem_geoInstDescs->data;
    descsRef.assign(desc, desc + instCount);
    idsRef.resize(instCount);
    
    for (size_t ii = 0; ii < instCount; ii++)
    {
        descsRef[ii].vertexBuffer = _brx.bufferIdxes[0];
        descsRef[ii].indexBuffer = _brx.bufferIdxes[1];
        descsRef[ii].maxCascade = c_brixelizerCascadeCount;

        idsRef[ii] = 0;
        descsRef[ii].outInstanceID = &idsRef[ii];
    }

    FFX_CHECK(ffxBrixelizerCreateInstances(&_brx.context, descsRef.data(), instCount));
}

void parseGeoBuf_n_submit(FFXBrixelizer_vk& _brx)
{
    if (nullptr == _brx.mem_geoBuf) {
        message(error, "invalid geo buffer!");
        return;
    }

    size_t count = _brx.mem_geoBuf->size / sizeof(BrixelBufDescs);
    if (2 != count) {
        message(error, "invalid geo buffer size!");
        return;
    }

    BrixelBufDescs* descs = (BrixelBufDescs*)_brx.mem_geoBuf->data;
    
    std::string nameStr[2] = { 
        "brixelizer vertex buffer",
        "brixelizer index buffer"
    };

    const uint32_t alignmnet = (uint32_t)s_renderVK->m_phyDeviceProps.limits.minStorageBufferOffsetAlignment;

    for (size_t ii = 0; ii < count; ii++)
    {
        FfxBrixelizerBufferDescription& refDesc = _brx.bufferDescs[ii];
        const Buffer_vk& kageBuf = s_renderVK->getBuffer(descs[ii].buf);

        refDesc.buffer.resource = kageBuf.buffer;
        refDesc.buffer.description.type = FFX_RESOURCE_TYPE_BUFFER;
        refDesc.buffer.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        refDesc.buffer.description.size = (uint32_t)kageBuf.size;
        refDesc.buffer.description.stride = descs[ii].stride;
        refDesc.buffer.description.alignment = alignmnet;
        refDesc.buffer.description.mipCount = 0;
        refDesc.buffer.description.flags = FFX_RESOURCE_FLAGS_NONE;
        refDesc.buffer.description.usage = FFX_RESOURCE_USAGE_READ_ONLY;
        refDesc.buffer.state = FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ;
        std::copy(nameStr[ii].begin(), nameStr[ii].end(), refDesc.buffer.name);

        _brx.bufferIdxes[ii] = 0;
        refDesc.outIndex = &_brx.bufferIdxes[ii];
    }

    FFX_CHECK(ffxBrixelizerRegisterBuffers(&_brx.context, _brx.bufferDescs, (uint32_t)count));
}

// set brixelizer buffers that required user provided data
// 0 debug descs
// 1 scratch buffer
// 2 sdf-atlas
// 3 brick-aabb
// [4, 4 + FFX_BRIXELIZER_MAX_CASCADES - 1]: cascade aabb trees
// [4 + FFX_BRIXELIZER_MAX_CASCADES, 4 + FFX_BRIXELIZER_MAX_CASCADES * 2 - 1]: cascade brick maps
// ------------------------------------------------------------------------------------------
void parseUserRes(FFXBrixelizer_vk& _brx)
{
    if (nullptr == _brx.mem_userRes) {
        message(error, "invalid user resources!");
        return;
    }
    size_t count = _brx.mem_userRes->size / sizeof(UnifiedResHandle);
    if (count != 5 + FFX_BRIXELIZER_MAX_CASCADES * 2) {
        message(error, "invalid user resources size!");
        return;
    }

    const UnifiedResHandle* handles = (UnifiedResHandle*)_brx.mem_userRes->data;

    // fill user resources of kage
    _brx.debugDestImg = handles[0].img;
    _brx.scratchBuf = handles[1].buf;
    _brx.userReses.sdfAtlas = handles[2].img;
    _brx.userReses.brickAABB = handles[3].buf;

    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        _brx.userReses.cascadeAABBTrees[ii] = handles[4 + ii].buf;
    }

    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        _brx.userReses.cascadeBrickMaps[ii] = handles[4 + FFX_BRIXELIZER_MAX_CASCADES + ii].buf;
    }

    _brx.userReses.cascadeInfos = handles[4 + FFX_BRIXELIZER_MAX_CASCADES * 2].buf;


    const uint32_t alignmnet = (uint32_t)s_renderVK->m_phyDeviceProps.limits.minStorageBufferOffsetAlignment;
    std::string nameStr;

    {
        // will store data to this ref
        FfxResource& atlasRef = _brx.updateDesc.resources.sdfAtlas;

        const Image_vk& sdfAtlasBuf = s_renderVK->getImage(_brx.userReses.sdfAtlas);

        atlasRef.resource = sdfAtlasBuf.image;
        atlasRef.description.type = FFX_RESOURCE_TYPE_TEXTURE3D;
        atlasRef.description.format = FFX_SURFACE_FORMAT_R8_UNORM;
        atlasRef.description.width = FFX_BRIXELIZER_STATIC_CONFIG_SDF_ATLAS_SIZE;
        atlasRef.description.height = FFX_BRIXELIZER_STATIC_CONFIG_SDF_ATLAS_SIZE;
        atlasRef.description.depth = FFX_BRIXELIZER_STATIC_CONFIG_SDF_ATLAS_SIZE;
        atlasRef.description.mipCount = 1;
        atlasRef.description.flags = FFX_RESOURCE_FLAGS_ALIASABLE;
        atlasRef.description.usage = FFX_RESOURCE_USAGE_UAV;
        atlasRef.state = FFX_RESOURCE_STATE_COMMON;
        nameStr = "brixelizer sdf atlas";
        std::copy(nameStr.begin(), nameStr.end(), atlasRef.name);
    }

    {
        // will store data to this ref
        FfxResource& brickAABBRef = _brx.updateDesc.resources.brickAABBs;

        const Buffer_vk& brickAABB = s_renderVK->getBuffer(_brx.userReses.brickAABB);

        brickAABBRef.resource = brickAABB.buffer;
        brickAABBRef.description.type = FFX_RESOURCE_TYPE_BUFFER;
        brickAABBRef.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        brickAABBRef.description.size = FFX_BRIXELIZER_BRICK_AABBS_SIZE;
        brickAABBRef.description.stride = FFX_BRIXELIZER_BRICK_AABBS_STRIDE;
        brickAABBRef.description.alignment = alignmnet;
        brickAABBRef.description.mipCount = 0;
        brickAABBRef.description.flags = FFX_RESOURCE_FLAGS_NONE;
        brickAABBRef.description.usage = FFX_RESOURCE_USAGE_UAV;
        brickAABBRef.state = FFX_RESOURCE_STATE_COMMON;
        nameStr = "brixelizer brick aabbs";
        std::copy(nameStr.begin(), nameStr.end(), brickAABBRef.name);
    }

    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        // will store data to this two refs
        FfxResource& cascadeAABBRef = _brx.updateDesc.resources.cascadeResources[ii].aabbTree;
        FfxResource& cascadeBrickMapRef = _brx.updateDesc.resources.cascadeResources[ii].brickMap;

        const Buffer_vk& cascadeAABB = s_renderVK->getBuffer(_brx.userReses.cascadeAABBTrees[ii]);

        cascadeAABBRef.resource = cascadeAABB.buffer;
        cascadeAABBRef.description.type = FFX_RESOURCE_TYPE_BUFFER;
        cascadeAABBRef.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        cascadeAABBRef.description.size = FFX_BRIXELIZER_CASCADE_AABB_TREE_SIZE;
        cascadeAABBRef.description.stride = FFX_BRIXELIZER_CASCADE_AABB_TREE_STRIDE;
        cascadeAABBRef.description.alignment = alignmnet;
        cascadeAABBRef.description.mipCount = 0;
        cascadeAABBRef.description.flags = FFX_RESOURCE_FLAGS_NONE;
        cascadeAABBRef.description.usage = FFX_RESOURCE_USAGE_UAV;
        cascadeAABBRef.state = FFX_RESOURCE_STATE_COMMON;
        nameStr = "brixelizer cascade aabb:" + std::to_string(ii);
        std::copy(nameStr.begin(), nameStr.end(), cascadeAABBRef.name);

        const Buffer_vk& cascadeBrickMap = s_renderVK->getBuffer(_brx.userReses.cascadeBrickMaps[ii]);

        cascadeBrickMapRef.resource = cascadeBrickMap.buffer;
        cascadeBrickMapRef.description.type = FFX_RESOURCE_TYPE_BUFFER;
        cascadeBrickMapRef.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        cascadeBrickMapRef.description.size = FFX_BRIXELIZER_CASCADE_BRICK_MAP_SIZE;
        cascadeBrickMapRef.description.stride = FFX_BRIXELIZER_CASCADE_BRICK_MAP_STRIDE;
        cascadeBrickMapRef.description.alignment = alignmnet;
        cascadeBrickMapRef.description.mipCount = 0;
        cascadeBrickMapRef.description.flags = FFX_RESOURCE_FLAGS_NONE;
        cascadeBrickMapRef.description.usage = FFX_RESOURCE_USAGE_UAV;
        cascadeBrickMapRef.state = FFX_RESOURCE_STATE_COMMON;
        nameStr = "brixelizer cascade brick map:" + std::to_string(ii);
        std::copy(nameStr.begin(), nameStr.end(), cascadeBrickMapRef.name);
    }
}

FfxBrixelizerTraceDebugModes getMode(BrixelDebugType _type)
{
    switch (_type)
    {
    case BrixelDebugType::distance:
        return FFX_BRIXELIZER_TRACE_DEBUG_MODE_DISTANCE;
    case BrixelDebugType::uvw:
        return FFX_BRIXELIZER_TRACE_DEBUG_MODE_UVW;
    case BrixelDebugType::iterations:
        return FFX_BRIXELIZER_TRACE_DEBUG_MODE_ITERATIONS;
    case BrixelDebugType::grad:
        return FFX_BRIXELIZER_TRACE_DEBUG_MODE_GRAD;
    case BrixelDebugType::brick_id:
        return FFX_BRIXELIZER_TRACE_DEBUG_MODE_BRICK_ID;
    case BrixelDebugType::cascade_id:
        return FFX_BRIXELIZER_TRACE_DEBUG_MODE_CASCADE_ID;
    default:
        return FFX_BRIXELIZER_TRACE_DEBUG_MODE_DISTANCE;
    }
}

void parseDebugVisDesc(FFXBrixelizer_vk& _brx)
{
    if (!_brx.mem_debugInfos
        || (_brx.mem_debugInfos->size != (uint32_t)sizeof(BrixelDebugDescs))) {
        message(warning, "invalid debug vis info");
        _brx.bDebug = false;
        return;
    }

    BrixelDebugDescs desc;
    memcpy(&desc, _brx.mem_debugInfos->data, sizeof(BrixelDebugDescs));

    
    FfxBrixelizerDebugVisualizationDescription& visDesc = _brx.debugVisDesc;

    FfxResource& debugDestImgRef = visDesc.output;
    const Image_vk& debugDestImgVk = s_renderVK->getImage(_brx.debugDestImg);

    debugDestImgRef.resource = debugDestImgVk.image;
    debugDestImgRef.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
    debugDestImgRef.description.format = FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    debugDestImgRef.description.width = debugDestImgVk.width;
    debugDestImgRef.description.height = debugDestImgVk.height;
    debugDestImgRef.description.depth = 1;
    debugDestImgRef.description.mipCount = 1;
    debugDestImgRef.description.flags = FFX_RESOURCE_FLAGS_ALIASABLE;
    debugDestImgRef.description.usage = FFX_RESOURCE_USAGE_UAV;
    debugDestImgRef.state = FFX_RESOURCE_STATE_COMMON;
    std::string nameStr = "brixelizer debug dest img";
    std::copy(nameStr.begin(), nameStr.end(), debugDestImgRef.name);

    memcpy(&visDesc.inverseViewMatrix[0], &desc.view[0], sizeof(visDesc.inverseViewMatrix));
    memcpy(&visDesc.inverseProjectionMatrix[0], &desc.proj[0], sizeof(visDesc.inverseProjectionMatrix));
    visDesc.debugState = getMode(desc.debugType);
    visDesc.startCascadeIndex = desc.start_cas;
    visDesc.endCascadeIndex = desc.end_cas;
    visDesc.sdfSolveEps = desc.sdf_eps;
    visDesc.tMax = desc.tmax;
    visDesc.tMin = desc.tmin;
    visDesc.renderWidth = debugDestImgVk.width;
    visDesc.renderHeight = debugDestImgVk.height;
    visDesc.commandList = s_renderVK->m_cmdBuffer;
    visDesc.numDebugAABBInstanceIDs = 0;
    visDesc.debugAABBInstanceIDs = nullptr;
    for (uint32_t  ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ii++) {
        visDesc.cascadeDebugAABB[ii] = FFX_BRIXELIZER_CASCADE_DEBUG_AABB_BOUNDING_BOX;
    }

    // set the center
    memcpy(&(_brx.initDesc.sdfCenter[0]), &desc.camPos, sizeof(_brx.initDesc.sdfCenter));

    _brx.bDebug = true;
}

void updateBrx(FFXBrixelizer_vk& _brx)
{
    size_t scratchBufferSize = 0;

    // update desc
    _brx.updateDesc.frameIndex = _brx.frameIdx;
    _brx.updateDesc.sdfCenter[0] = _brx.initDesc.sdfCenter[0];
    _brx.updateDesc.sdfCenter[1] = _brx.initDesc.sdfCenter[1];
    _brx.updateDesc.sdfCenter[2] = _brx.initDesc.sdfCenter[2];
    _brx.updateDesc.populateDebugAABBsFlags = FFX_BRIXELIZER_POPULATE_AABBS_CASCADE_AABBS;
    _brx.updateDesc.debugVisualizationDesc = _brx.bDebug ? &_brx.debugVisDesc : nullptr;
    _brx.updateDesc.maxReferences = 32 * (1 << 20);
    _brx.updateDesc.maxBricksPerBake = 1 << 14;
    _brx.updateDesc.triangleSwapSize = 300 * (1 << 20);
    _brx.updateDesc.outStats = &_brx.stats;
    _brx.updateDesc.outScratchBufferSize = &scratchBufferSize;

    // update brixelizer bake
    FFX_CHECK(ffxBrixelizerBakeUpdate(&_brx.context, &_brx.updateDesc, &_brx.bakedUpdateDesc));

    if (scratchBufferSize > _brx.gpuScratchedBufferSize)
    {
        s_renderVK->updateBuffer(_brx.scratchBuf, nullptr, 0u, (uint32_t)scratchBufferSize);
        _brx.gpuScratchedBufferSize = scratchBufferSize;
    }

    const uint32_t alignmnet = (uint32_t)s_renderVK->m_phyDeviceProps.limits.minStorageBufferOffsetAlignment;
    FfxResource scratchRes;
    {
        const Buffer_vk& scratchBuf = s_renderVK->getBuffer(_brx.scratchBuf);

        scratchRes.resource = scratchBuf.buffer;
        scratchRes.description.type = FFX_RESOURCE_TYPE_BUFFER;
        scratchRes.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        scratchRes.description.size = (uint32_t)scratchBufferSize;
        scratchRes.description.stride = sizeof(uint32_t);
        scratchRes.description.alignment = alignmnet;
        scratchRes.description.mipCount = 0;
        scratchRes.description.flags = FFX_RESOURCE_FLAGS_ALIASABLE;
        scratchRes.description.usage = FFX_RESOURCE_USAGE_UAV;
        scratchRes.state = FFX_RESOURCE_STATE_UNORDERED_ACCESS;
        std::string nameStr = "brixelizer scratch buffer";
        std::copy(nameStr.begin(), nameStr.end(), scratchRes.name);
    }

    FFX_CHECK(ffxBrixelizerUpdate(&_brx.context, &_brx.bakedUpdateDesc, scratchRes, s_renderVK->m_cmdBuffer));

    _brx.frameIdx++;
}

void init_ffx(FFXBrixelizer_vk::FfxCtx& _ffx)
{
    // Create the FFX context
    constexpr uint32_t c_maxContexts = 2;
    size_t scratchMemSz = ffxGetScratchMemorySizeVK(s_renderVK->m_physicalDevice, c_maxContexts);
    const Memory* scratchMem = kage::alloc((uint32_t)scratchMemSz);
    memset(scratchMem->data, 0, scratchMem->size);

    VkDeviceContext vkdevCtx;
    vkdevCtx.vkDevice = s_renderVK->m_device;
    vkdevCtx.vkPhysicalDevice = s_renderVK->m_physicalDevice;
    vkdevCtx.vkDeviceProcAddr = s_renderVK->m_vkGetDeviceProcAddr;

    FfxDevice ffxDevice = ffxGetDeviceVK(&vkdevCtx);
    FfxInterface interface;
    FfxErrorCode err = ffxGetInterfaceVK(&interface, ffxDevice, scratchMem->data, scratchMem->size, c_maxContexts);

    assert(err == FFX_OK);

    _ffx.scratchMem = scratchMem;
    _ffx.device = ffxDevice;
    _ffx.interface = interface;
}

void init(FFXBrixelizer_vk& _brx)
{
    init_ffx(_brx.ffxCtx);
    createCtx(_brx);
}

void initAfterCmdReady(FFXBrixelizer_vk& _brx)
{
    parseGeoBuf_n_submit(_brx);
    parseGeoInst_n_submit(_brx);
}

void preUpdateBarriers(FFXBrixelizer_vk& _brx)
{
    BarrierDispatcher& dispatcher = s_renderVK->m_barrierDispatcher;

    if (_brx.bDebug)
    {
        const Image_vk& img = s_renderVK->getImage(_brx.debugDestImg);
        dispatcher.barrier(img.image,
            VK_IMAGE_ASPECT_COLOR_BIT, {
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT
                , VK_IMAGE_LAYOUT_GENERAL
                , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
            });
    }

    // scratch buffer
    {
        const Buffer_vk& buf = s_renderVK->getBuffer(_brx.scratchBuf);
        dispatcher.barrier(buf.buffer, {
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT
                , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
            });
    }

    // sdf atlas
    {
        const Image_vk& img = s_renderVK->getImage(_brx.userReses.sdfAtlas);
        dispatcher.barrier(img.image, 
            VK_IMAGE_ASPECT_COLOR_BIT, {
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT
                , VK_IMAGE_LAYOUT_GENERAL
                , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
            });
    }

    // brick aabb
    {
        const Buffer_vk& buf = s_renderVK->getBuffer(_brx.userReses.brickAABB);
        dispatcher.barrier(buf.buffer, {
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT
                , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
            });
    }

    // cascades
    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        const Buffer_vk& aabbTree = s_renderVK->getBuffer(_brx.userReses.cascadeAABBTrees[ii]);
        dispatcher.barrier(aabbTree.buffer, {
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT
                , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
            });
        const Buffer_vk& brickMap = s_renderVK->getBuffer(_brx.userReses.cascadeBrickMaps[ii]);
        dispatcher.barrier(brickMap.buffer, {
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT
                , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
            });
    }

    dispatcher.dispatch(s_renderVK->m_cmdBuffer);
}

void updateContextInfo(FFXBrixelizer_vk& _brx)
{
    FfxBrixelizerContextInfo contextInfo = {};
    FFX_CHECK(ffxBrixelizerGetContextInfo(&_brx.context, &contextInfo));

    const Memory* mem = alloc(sizeof(FfxBrixelizerCascadeInfo) * FFX_BRIXELIZER_MAX_CASCADES);
    memcpy(mem->data, contextInfo.cascades, mem->size);

    // update the context info to user res
    const Buffer_vk& cascadeInfos = s_renderVK->getBuffer(_brx.userReses.cascadeInfos);
    s_renderVK->updateBuffer(_brx.userReses.cascadeInfos, mem, 0u, mem->size);

    release (mem);
}

void update(FFXBrixelizer_vk& _brx)
{
    static bool s_init = false;
    if (!s_init)
    {
        s_init = true;
        initAfterCmdReady(_brx);
        parseUserRes(_brx);
    }

    parseDebugVisDesc(_brx);
    preUpdateBarriers(_brx);
    updateContextInfo(_brx);
    updateBrx(_brx);


}

void deleteInstances(FFXBrixelizer_vk& _brx)
{
    stl::vector<FfxBrixelizerInstanceID> instIds;
    for (FfxBrixelizerInstanceID& inst : _brx.geoInstIds)
    {
        if (inst == FFX_BRIXELIZER_INVALID_ID)
            continue;
        instIds.push_back(inst);
        inst = FFX_BRIXELIZER_INVALID_ID;
    }
    if (instIds.size() > 0) {
        FFX_CHECK(ffxBrixelizerDeleteInstances(&_brx.context, instIds.data(), (uint32_t)instIds.size()));
    }
}

void deleteBrixelizerContext(FFXBrixelizer_vk& _brx)
{
    FFX_CHECK(ffxBrixelizerContextDestroy(&_brx.context));
}

void shutdown(FFXBrixelizer_vk& _brx)
{
    deleteInstances(_brx);
    deleteBrixelizerContext(_brx);

    // shutdown ffx
    _brx.ffxCtx.interface = {};
    _brx.ffxCtx.device = {};
    release(_brx.ffxCtx.scratchMem);
}

} // namespace brx
} // namespace vk
} // namespace kage