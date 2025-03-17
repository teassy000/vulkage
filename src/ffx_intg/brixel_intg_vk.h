#pragma once

#include "kage.h"
#include "kage_inner.h"
#include "common.h" // for stl
#include "FidelityFX/host/ffx_brixelizer.h"

namespace kage { namespace vk { namespace bxl
{
    struct FFXBrixelizer_vk
    {
        BufferHandle scratchBuf;
        ImageHandle sdfAtlas;
        BufferHandle brickAABB;
        BufferHandle cascadeAABBTrees[FFX_BRIXELIZER_MAX_CASCADES];
        BufferHandle cascadeBrickMaps[FFX_BRIXELIZER_MAX_CASCADES];
    
        BufferHandle vtxBuf;
        BufferHandle idxBuf;

        FfxBrixelizerContext context;
        FfxBrixelizerContextDescription initDesc;
        FfxBrixelizerBakedUpdateDescription bakedUpdateDesc;
        FfxBrixelizerUpdateDescription updateDesc;

        stl::vector<FfxBrixelizerInstanceDescription> geoInstDescs;
        stl::vector<FfxBrixelizerInstanceID> geoInstIds;

        // 0 - vertex buffer
        // 1 - index buffer
        FfxBrixelizerBufferDescription bufferDescs[2]{};
        uint32_t bufferIdxes[2]{};

        const Memory* mem_geoInstDescs{ nullptr };
        const Memory* mem_geoBuf{ nullptr };
        const Memory* mem_userRes{ nullptr };

        bool postInitilized{ false };
        uint32_t frameIdx{ 0 };
        float sdfCenter[3]{ 0.f, 0.f, 0.f };
        size_t gpuScratchedBufferSize{ 128 * 1024 * 1024 }; // 128MB

        struct FfxCtx
        {
            const Memory* scratchMem;
            FfxDevice device;
            FfxInterface interface;
        }ffxCtx;

    };

    void init(FFXBrixelizer_vk&);
    void initAfterCmdReady(FFXBrixelizer_vk&);
    void update(FFXBrixelizer_vk&);
    void shutdown(FFXBrixelizer_vk&);

    void setGeoInstances(FFXBrixelizer_vk& _bxl, const Memory* _desc);
    void setGeoBuffers(FFXBrixelizer_vk& _bxl, const Memory* _bufs);
    void setUserResources(FFXBrixelizer_vk& _bxl, const Memory* _reses);
} // namespace kage::vk::bxl
} // namespace kage::vk
} // namespace kage