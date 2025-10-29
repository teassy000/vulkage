#pragma once

#include "core/kage.h"
#include "core/kage_math.h"
#include "demo_structs.h"

struct SoftRasterDataInit
{
    // readonly
    kage::BufferHandle payloadBuf;
    kage::BufferHandle payloadCntBuf;

    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transformBuf;
    kage::BufferHandle vtxBuf;
    kage::BufferHandle meshletBuf;
    kage::BufferHandle meshletDataBuf;

    kage::ImageHandle pyramid;
    
    uint32_t width; // width of the output image
    uint32_t height; // height of the output image

    kage::ImageHandle color; // output image for soft rasterization results
    kage::ImageHandle depth; // output depth image for soft rasterization results

    RenderStage renderStage;
};

struct SoftRaster
{
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;
    kage::PassHandle pass;

    // read-only resources
    kage::BufferHandle payloadBuf;
    kage::BufferHandle payloadCntBuf;

    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transformBuf;
    kage::BufferHandle vtxBuf;
    kage::BufferHandle meshletBuf;
    kage::BufferHandle meshletDataBuf;

    kage::ImageHandle pyramid; // input depth image for soft rasterization
    kage::SamplerHandle pyramidSamp; // sampler for the input image

    kage::ImageHandle inColor;
    kage::ImageHandle inDepth;
    
    kage::ImageHandle u32depth; // depth image in uint32_t format for soft rasterization
    kage::ImageHandle u32depthOutAlias;

    kage::ImageHandle u32debugImg; // debug image in uint32_t format for soft rasterization
    kage::ImageHandle u32debugImgOutAlias;
    
    // output alias
    kage::ImageHandle colorOutAlias; // output image for soft rasterization results
    kage::ImageHandle depthOutAlias; // output depth image for soft rasterization results

    // other essential data
    uint32_t width; // width of the output image
    uint32_t height; // height of the output image

    RenderStage renderStage;
};


struct ModifySoftRasterCmd
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;

    uint32_t width;
    uint32_t height;

    kage::BufferHandle inPayloadCntBuf;
    kage::BufferHandle payloadCntBufOutAlias;
};


void initSoftRaster(SoftRaster& _softRaster, const SoftRasterDataInit& _initData);
void updateSoftRaster(SoftRaster& _softRaster);