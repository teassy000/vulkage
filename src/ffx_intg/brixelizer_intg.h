#pragma once

#include "FidelityFX/host/ffx_brixelizer.h"
#include "kage.h"
#include <vector>


struct BrixelizerInstInfo
{

};

struct BrielizerBufInfo
{

};

struct FFX_Brixelizer_Impl
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;


    kage::ImageHandle sdfAtlas;
    kage::BufferHandle brickAABB;
    kage::BufferHandle cascadeAABBTrees[FFX_BRIXELIZER_MAX_CASCADES];
    kage::BufferHandle cascadeBrickMaps[FFX_BRIXELIZER_MAX_CASCADES];

    FfxBrixelizerContextDescription initDesc;
    FfxBrixelizerContext context;
    FfxBrixelizerBakedUpdateDescription updateDesc;

    std::vector<BrixelizerInstInfo> insts;
    std::vector<BrielizerBufInfo> bufs;
};

void initBrixelizerImpl(FFX_Brixelizer_Impl& _ffx);
void updateBrixellizerImpl(const FFX_Brixelizer_Impl& _ffx);