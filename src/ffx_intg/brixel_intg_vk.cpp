#include "brixel_intg_vk.h"
#include "debug.h"
#include "kage_math.h"
#include "brixel_structs.h"

#include "common.h" // for stl

#include "FidelityFX/host/backends/vk/ffx_vk.h" // for init_ffx

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_major_storage.hpp>
#include "rhi_context_vk.h" // for s_renderVK

namespace kage { namespace vk {

extern RHIContext_vk* s_renderVK; // accessing Vk context

namespace bxl {

#define FFX_CHECK(err) if (err != FFX_OK) \
    { kage::message(kage::error, "ffx check failed with error code %d.", err); }

constexpr uint32_t c_brixelizerCascadeCount = (FFX_BRIXELIZER_MAX_CASCADES / 3);

struct BrixelizerConfig
{
    float meshUnitSize{ .2f };
    float cascadeSizeRatio{ 2.f };
};

const static BrixelizerConfig s_conf{};

void createCtx(FFXBrixelizer_vk& _blx)
{
    _blx.initDesc.sdfCenter[0] = 0.f;
    _blx.initDesc.sdfCenter[1] = 0.f;
    _blx.initDesc.sdfCenter[2] = 0.f;
    _blx.initDesc.numCascades = c_brixelizerCascadeCount;
    _blx.initDesc.flags = FFX_BRIXELIZER_CONTEXT_FLAG_ALL_DEBUG;

    float voxSize = s_conf.meshUnitSize;
    for (uint32_t ii = 0; ii < _blx.initDesc.numCascades; ++ii)
    {
        FfxBrixelizerCascadeDescription& refDesc = _blx.initDesc.cascadeDescs[ii];

        refDesc.flags = (FfxBrixelizerCascadeFlag)(FFX_BRIXELIZER_CASCADE_STATIC | FFX_BRIXELIZER_CASCADE_DYNAMIC);
        refDesc.voxelSize = voxSize;

        voxSize *= s_conf.cascadeSizeRatio;
    }

    _blx.initDesc.backendInterface = _blx.ffxCtx.interface;

    FFX_CHECK(ffxBrixelizerContextCreate(&_blx.initDesc, &_blx.context));
}

void setGeoInstances(FFXBrixelizer_vk& _blx, const Memory* _descs)
{
    if (nullptr == _descs) {
        message(error, "invalid _desc ptr!");
        return;
    }

    _blx.mem_geoInstDescs = _descs;
}

void setGeoBuffers(FFXBrixelizer_vk& _blx, const Memory* _buf)
{
    if (nullptr == _buf) {
        message(error, "invalid _buffers ptr!");
        return;
    }

    _blx.mem_geoBuf = _buf;
}

void setUserResources(FFXBrixelizer_vk& _blx, const Memory* _reses)
{
    if (nullptr == _reses) {
        message(error, "invalid _reses ptr!");
        return;
    }

    _blx.mem_userRes = _reses;
}

void parseGeoInst_n_submit(FFXBrixelizer_vk& _blx)
{
    if (nullptr == _blx.mem_geoInstDescs) {
        message(error, "invalid geo instance descs!");
        return;
    }
    
    uint32_t instCount = _blx.mem_geoInstDescs->size / sizeof(FfxBrixelizerInstanceDescription);

    if (instCount == 0) {
        message(error, "invalid geo instance count!");
        return;
    }

    stl::vector<FfxBrixelizerInstanceDescription>& descsRef = _blx.geoInstDescs;
    stl::vector<FfxBrixelizerInstanceID>& idsRef = _blx.geoInstIds; 

    FfxBrixelizerInstanceDescription* desc = (FfxBrixelizerInstanceDescription*)_blx.mem_geoInstDescs->data;
    descsRef.assign(desc, desc + instCount);
    idsRef.resize(instCount);
    
    for (size_t ii = 0; ii < instCount; ii++)
    {
        idsRef[ii] = 0;
        descsRef[ii].outInstanceID = &idsRef[ii];
    }

    FFX_CHECK(ffxBrixelizerCreateInstances(&_blx.context, descsRef.data(), instCount));
}

void parseGeoBuf_n_submit(FFXBrixelizer_vk& _blx)
{
    if (nullptr == _blx.mem_geoBuf) {
        message(error, "invalid geo buffer!");
        return;
    }

    size_t count = _blx.mem_geoBuf->size / sizeof(BrixelBufDescs);
    if (2 != count) {
        message(error, "invalid geo buffer size!");
        return;
    }

    BrixelBufDescs* descs = (BrixelBufDescs*)_blx.mem_geoBuf->data;
    
    const BrixelBufDescs& vtxDesc = descs[0];
    const BrixelBufDescs& idxDesc = descs[1];
    
    std::string nameStr[2] = { 
        "brixelizer vertex buffer",
        "brixelizer index buffer"
    };

    for (size_t ii = 0; ii < count; ii++)
    {
        FfxBrixelizerBufferDescription& refDesc = _blx.bufferDescs[ii];
        const Buffer_vk& kageBuf = s_renderVK->getBuffer(descs[ii].buf);

        refDesc.buffer.resource = kageBuf.buffer;
        refDesc.buffer.description.type = FFX_RESOURCE_TYPE_BUFFER;
        refDesc.buffer.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        refDesc.buffer.description.size = (uint32_t)kageBuf.size;
        refDesc.buffer.description.stride = descs[ii].stride;
        refDesc.buffer.description.alignment = 0;
        refDesc.buffer.description.mipCount = 0;
        refDesc.buffer.description.flags = FFX_RESOURCE_FLAGS_NONE;
        refDesc.buffer.description.usage = FFX_RESOURCE_USAGE_READ_ONLY;
        refDesc.buffer.state = FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ;
        std::copy(nameStr[ii].begin(), nameStr[ii].end(), refDesc.buffer.name);

        _blx.bufferIdxes[ii] = 0;
        refDesc.outIndex = &_blx.bufferIdxes[ii];
    }

    FFX_CHECK(ffxBrixelizerRegisterBuffers(&_blx.context, _blx.bufferDescs, (uint32_t)count));
}

// set brixelizer buffers that required user provided data
// 0 scratch buffer
// 1 sdf-atlas
// 2 brick-aabb
// [3, 3 + FFX_BRIXELIZER_MAX_CASCADES - 1]: cascade aabb trees
// [3 + FFX_BRIXELIZER_MAX_CASCADES, 3 + FFX_BRIXELIZER_MAX_CASCADES * 2 - 1]: cascade brick maps
// ------------------------------------------------------------------------------------------
void parseUserRes(FFXBrixelizer_vk& _blx)
{
    if (nullptr == _blx.mem_userRes) {
        message(error, "invalid user resources!");
        return;
    }
    size_t count = _blx.mem_userRes->size / sizeof(UnifiedResHandle);
    if (count != 3 + FFX_BRIXELIZER_MAX_CASCADES * 2) {
        message(error, "invalid user resources size!");
        return;
    }

    const UnifiedResHandle* handles = (UnifiedResHandle*)_blx.mem_userRes->data;

    // fill user resources of kage
    _blx.scratchBuf = handles[0].buf;
    _blx.sdfAtlas = handles[1].img;
    _blx.brickAABB = handles[2].buf;

    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        _blx.cascadeAABBTrees[ii] = handles[3 + ii].buf;
    }

    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        _blx.cascadeBrickMaps[ii] = handles[3 + FFX_BRIXELIZER_MAX_CASCADES + ii].buf;
    }

    std::string nameStr;
    {
        // will store data to this ref
        FfxResource& atlasRef = _blx.updateDesc.resources.sdfAtlas;

        const Image_vk& sdfAtlasBuf = s_renderVK->getImage(_blx.sdfAtlas);

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
        FfxResource& brickAABBRef = _blx.updateDesc.resources.brickAABBs;

        const Buffer_vk& brickAABB = s_renderVK->getBuffer(_blx.brickAABB);

        brickAABBRef.resource = brickAABB.buffer;
        brickAABBRef.description.type = FFX_RESOURCE_TYPE_BUFFER;
        brickAABBRef.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        brickAABBRef.description.size = FFX_BRIXELIZER_BRICK_AABBS_SIZE;
        brickAABBRef.description.stride = FFX_BRIXELIZER_BRICK_AABBS_STRIDE;
        brickAABBRef.description.alignment = 0;
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
        FfxResource& cascadeAABBRef = _blx.updateDesc.resources.cascadeResources[ii].aabbTree;
        FfxResource& cascadeBrickMapRef = _blx.updateDesc.resources.cascadeResources[ii].brickMap;

        const Buffer_vk& cascadeAABB = s_renderVK->getBuffer(_blx.cascadeAABBTrees[ii]);

        cascadeAABBRef.resource = cascadeAABB.buffer;
        cascadeAABBRef.description.type = FFX_RESOURCE_TYPE_BUFFER;
        cascadeAABBRef.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        cascadeAABBRef.description.size = FFX_BRIXELIZER_CASCADE_AABB_TREE_SIZE;
        cascadeAABBRef.description.stride = FFX_BRIXELIZER_CASCADE_AABB_TREE_STRIDE;
        cascadeAABBRef.description.alignment = 0;
        cascadeAABBRef.description.mipCount = 0;
        cascadeAABBRef.description.flags = FFX_RESOURCE_FLAGS_NONE;
        cascadeAABBRef.description.usage = FFX_RESOURCE_USAGE_UAV;
        cascadeAABBRef.state = FFX_RESOURCE_STATE_COMMON;
        nameStr = "brixelizer cascade aabb:" + std::to_string(ii);
        std::copy(nameStr.begin(), nameStr.end(), cascadeAABBRef.name);

        const Buffer_vk& cascadeBrickMap = s_renderVK->getBuffer(_blx.cascadeBrickMaps[ii]);

        cascadeBrickMapRef.resource = cascadeBrickMap.buffer;
        cascadeBrickMapRef.description.type = FFX_RESOURCE_TYPE_BUFFER;
        cascadeBrickMapRef.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        cascadeBrickMapRef.description.size = FFX_BRIXELIZER_CASCADE_BRICK_MAP_SIZE;
        cascadeBrickMapRef.description.stride = FFX_BRIXELIZER_CASCADE_BRICK_MAP_STRIDE;
        cascadeBrickMapRef.description.alignment = 0;
        cascadeBrickMapRef.description.mipCount = 0;
        cascadeBrickMapRef.description.flags = FFX_RESOURCE_FLAGS_NONE;
        cascadeBrickMapRef.description.usage = FFX_RESOURCE_USAGE_UAV;
        cascadeBrickMapRef.state = FFX_RESOURCE_STATE_COMMON;
        nameStr = "brixelizer cascade brick map:" + std::to_string(ii);
        std::copy(nameStr.begin(), nameStr.end(), cascadeBrickMapRef.name);
    }
}


void updateBlx(FFXBrixelizer_vk& _blx)
{
    FfxBrixelizerStats stats = {};
    size_t scratchBufferSize = 0;

    _blx.updateDesc.frameIndex = _blx.frameIdx;
    _blx.updateDesc.sdfCenter[0] = _blx.initDesc.sdfCenter[0];
    _blx.updateDesc.sdfCenter[1] = _blx.initDesc.sdfCenter[1];
    _blx.updateDesc.sdfCenter[2] = _blx.initDesc.sdfCenter[2];
    _blx.updateDesc.populateDebugAABBsFlags = FFX_BRIXELIZER_POPULATE_AABBS_NONE;
    _blx.updateDesc.debugVisualizationDesc = nullptr;
    _blx.updateDesc.maxReferences = 32 * (1 << 20);
    _blx.updateDesc.maxBricksPerBake = 1 << 14;
    _blx.updateDesc.triangleSwapSize = 300 * (1 << 20);
    _blx.updateDesc.outStats = &stats;
    _blx.updateDesc.outScratchBufferSize = &scratchBufferSize;

    // update brixelizer bake
    FFX_CHECK(ffxBrixelizerBakeUpdate(&_blx.context, &_blx.updateDesc, &_blx.bakedUpdateDesc));

    if (scratchBufferSize > _blx.gpuScratchedBufferSize)
    {
        s_renderVK->updateBuffer(_blx.scratchBuf, nullptr, 0u, (uint32_t)scratchBufferSize);
        _blx.gpuScratchedBufferSize = scratchBufferSize;
    }

    FfxResource scratchRes;
    {
        const Buffer_vk& scratchBuf = s_renderVK->getBuffer(_blx.scratchBuf);

        scratchRes.resource = scratchBuf.buffer;
        scratchRes.description.type = FFX_RESOURCE_TYPE_BUFFER;
        scratchRes.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
        scratchRes.description.size = (uint32_t)scratchBufferSize;
        scratchRes.description.stride = sizeof(uint32_t);
        scratchRes.description.alignment = 0;
        scratchRes.description.mipCount = 0;
        scratchRes.description.flags = FFX_RESOURCE_FLAGS_NONE;
        scratchRes.description.usage = FFX_RESOURCE_USAGE_UAV;
        scratchRes.state = FFX_RESOURCE_STATE_UNORDERED_ACCESS;
        std::string nameStr = "brixelizer scratch buffer";
        std::copy(nameStr.begin(), nameStr.end(), scratchRes.name);
    }

    FFX_CHECK(ffxBrixelizerUpdate(&_blx.context, &_blx.bakedUpdateDesc, scratchRes, s_renderVK->m_cmdBuffer));
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

void init(FFXBrixelizer_vk& _blx)
{
    init_ffx(_blx.ffxCtx);
    createCtx(_blx);
}

void initAfterCmdReady(FFXBrixelizer_vk& _blx)
{
    static bool s_init = false;
    if (!s_init)
    {
        s_init = true;
        parseGeoInst_n_submit(_blx);
        parseGeoBuf_n_submit(_blx);
    }
}


void preUpdateBarriers(FFXBrixelizer_vk& _bxl)
{
    BarrierDispatcher& dispatcher = s_renderVK->m_barrierDispatcher;

    // scratch buffer
    {
        const Buffer_vk& buf = s_renderVK->getBuffer(_bxl.scratchBuf);
        dispatcher.barrier(buf.buffer, {
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT
                , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
            });
    }

    // sdf atlas
    {
        const Image_vk& img = s_renderVK->getImage(_bxl.sdfAtlas);
        dispatcher.barrier(img.image, 
            VK_IMAGE_ASPECT_COLOR_BIT, {
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT
                , VK_IMAGE_LAYOUT_GENERAL
                , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
            });
    }

    // brick aabb
    {
        const Buffer_vk& buf = s_renderVK->getBuffer(_bxl.brickAABB);
        dispatcher.barrier(buf.buffer, {
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT
                , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
            });
    }

    // cascades
    for (uint32_t ii = 0; ii < FFX_BRIXELIZER_MAX_CASCADES; ++ii)
    {
        const Buffer_vk& aabbTree = s_renderVK->getBuffer(_bxl.cascadeAABBTrees[ii]);
        dispatcher.barrier(aabbTree.buffer, {
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT
                , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
            });
        const Buffer_vk& brickMap = s_renderVK->getBuffer(_bxl.cascadeBrickMaps[ii]);
        dispatcher.barrier(brickMap.buffer, {
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT
                , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
            });
    }

    dispatcher.dispatch(s_renderVK->m_cmdBuffer);
}

void update(FFXBrixelizer_vk& _blx)
{
    parseUserRes(_blx);

    preUpdateBarriers(_blx);
    updateBlx(_blx);
}

void deleteInstances(FFXBrixelizer_vk& _blx)
{
    stl::vector<FfxBrixelizerInstanceID> instIds;
    for (FfxBrixelizerInstanceID& inst : _blx.geoInstIds)
    {
        if (inst == FFX_BRIXELIZER_INVALID_ID)
            continue;
        instIds.push_back(inst);
        inst == FFX_BRIXELIZER_INVALID_ID;
    }
    if (instIds.size() > 0) {
        FFX_CHECK(ffxBrixelizerDeleteInstances(&_blx.context, instIds.data(), (uint32_t)instIds.size()));
    }
}

void deleteBrixelizerContext(FFXBrixelizer_vk& _blx)
{
    FFX_CHECK(ffxBrixelizerContextDestroy(&_blx.context));
}



void shutdown(FFXBrixelizer_vk& _blx)
{
    deleteInstances(_blx);
    deleteBrixelizerContext(_blx);

    // shutdown ffx
    _blx.ffxCtx.interface = {};
    _blx.ffxCtx.device = {};
    release(_blx.ffxCtx.scratchMem);
}


} // namespace bxl
} // namespace vk
} // namespace kage