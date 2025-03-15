#include "kage.h"
#include "scene.h"
#include "FidelityFX/host/ffx_brixelizer.h"

struct BrixelResources
{
    // expose following data which would be use in other passes
    kage::BufferHandle scratchBuf;
    kage::ImageHandle sdfAtlas;
    kage::BufferHandle brickAABB;
    kage::BufferHandle cascadeAABBTrees[FFX_BRIXELIZER_MAX_CASCADES];
    kage::BufferHandle cascadeBrickMaps[FFX_BRIXELIZER_MAX_CASCADES];
};

struct BrixelInitDesc
{
    kage::BufferHandle vtxBuf;
    uint32_t vtxSz;
    uint32_t vtxStride;
    kage::BufferHandle idxBuf;
    uint32_t idxSz;
    uint32_t idxStride;

    bool seamless;
};


void bxlInit(BrixelResources& _bxl, const BrixelInitDesc& _init, const Scene& _scene);