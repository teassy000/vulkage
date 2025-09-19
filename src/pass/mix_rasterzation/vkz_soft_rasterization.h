#pragma once

#include "core/kage.h"
#include "core/kage_math.h"
#include "demo_structs.h"

struct SoftRasterizationDataInit
{
    // from triangle culling pass
    kage::BufferHandle triangleBuf; 
    kage::BufferHandle vtxBuf; 
    kage::BufferHandle payloadCountBuf;

    kage::ImageHandle pyramid; // input depth image for soft rasterization
    
    uint32_t width; // width of the output image
    uint32_t height; // height of the output image

    kage::ImageHandle color; // output image for soft rasterization results
    kage::ImageHandle depth; // output depth image for soft rasterization results

    RenderStage renderStage;
};

struct SoftRasterization
{
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;
    kage::PassHandle pass;

    // read-only resources
    kage::BufferHandle inTriangleBuf;
    kage::BufferHandle inVtxBuf;
    kage::BufferHandle inPayloadCountBuf;

    kage::ImageHandle pyramid; // input depth image for soft rasterization
    kage::SamplerHandle pyramidSamp; // sampler for the input image

    kage::ImageHandle inColor;
    kage::ImageHandle inDepth;
    
    kage::ImageHandle u32depth; // depth image in uint32_t format for soft rasterization
    kage::ImageHandle u32depthOutAlias; // 
    
    // output alias
    kage::ImageHandle colorOutAlias; // output image for soft rasterization results
    kage::ImageHandle depthOutAlias; // output depth image for soft rasterization results

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


void initSoftRasterization(SoftRasterization& _softRaster, const SoftRasterizationDataInit& _initData);
void updateSoftRasterization(SoftRasterization& _softRaster);

void initModifySoftRasterCmd(ModifySoftRasterCmd& _cmd, const kage::BufferHandle& _triangleCountBuf, uint32_t _width, uint32_t _height);
void updateModifySoftRasterCmd(ModifySoftRasterCmd& _cmd, uint32_t _width, uint32_t _height);