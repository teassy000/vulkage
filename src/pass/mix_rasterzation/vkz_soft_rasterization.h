#pragma once

#include "core/kage.h"
#include "core/kage_math.h"

struct SoftRasterizationDataInit
{
    kage::BufferHandle transformBuf; // for transform data
    kage::BufferHandle meshletVisBuf; // meshlet visibility buffer
    kage::BufferHandle meshletBuffer; // meshlet buffer
    kage::BufferHandle meshletDataBuffer; // meshlet data buffer
    
    uint32_t width; // width of the output image
    uint32_t height; // height of the output image

    kage::ImageHandle color; // output image for soft rasterization results
    kage::ImageHandle depth; // output depth image for soft rasterization results
};

struct SoftRasterization
{
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;
    kage::PassHandle pass;

    // read-only resources
    kage::BufferHandle transformBuf;

    kage::ImageHandle inPyramid; // input depth image for soft rasterization
    kage::SamplerHandle pyramidSamp; // sampler for the input image

    kage::ImageHandle inColor;
    kage::ImageHandle inDepth;
    // output alias
    kage::ImageHandle colorOutAlias; // output image for soft rasterization results
    kage::ImageHandle depthOutAlias; // output depth image for soft rasterization results

    uint32_t width; // width of the output image
    uint32_t height; // height of the output image
};

void initSoftRasterization(SoftRasterization& _softRaster, const SoftRasterizationDataInit& _initData);

void updateSoftRasterization(SoftRasterization& _softRaster);