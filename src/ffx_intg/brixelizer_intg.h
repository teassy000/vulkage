#pragma once

#include "FidelityFX/host/ffx_brixelizer.h"
#include "kage.h"
#include "scene.h"
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
    kage::BufferHandle vtxBuf;
    kage::BufferHandle idxBuf;

    FfxBrixelizerContextDescription initDesc;
    FfxBrixelizerContext context;
    FfxBrixelizerBakedUpdateDescription updateDesc;

    std::vector<BrixelizerInstInfo> insts;
    std::vector<BrielizerBufInfo> bufs;
};

void initBrixelizerImpl(FFX_Brixelizer_Impl& _ffx, const kage::BufferHandle _vtxBuf, const kage::BufferHandle _idxBuf);
void postInitBrixelizerImpl(FFX_Brixelizer_Impl& _ffx, const Scene& _scene);
void updateBrixellizerImpl(const FFX_Brixelizer_Impl& _ffx);