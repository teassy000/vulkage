#pragma once

#include "core/kage.h"
#include "deferred/vkz_deferred.h"


struct HardRasterInitData
{
    kage::BufferHandle vtxBuffer;
    kage::BufferHandle meshBuffer;
    kage::BufferHandle meshletBuffer;
    kage::BufferHandle meshletDataBuffer;
    kage::BufferHandle meshDrawBuffer;
    kage::BufferHandle transformBuffer;

    kage::BufferHandle triPayloadBuffer;
    kage::BufferHandle triPayloadCountBuffer;

    kage::ImageHandle pyramid;
    kage::ImageHandle depth;

    kage::BindlessHandle bindless;

    GBuffer g_buffer;
};

// using mesh shading pipeline for hardware rasterization
struct HardRaster
{
    RenderPipeline pipeline{ RenderPipeline::mixed };
    PassStage stage{ PassStage::early };

    kage::PassHandle pass;
    
    kage::ShaderHandle ms;
    kage::ShaderHandle fs;
    kage::ProgramHandle program;
    
    // read-only
    kage::BufferHandle vtxBuffer;
    kage::BufferHandle meshBuffer;
    kage::BufferHandle meshletBuffer;
    kage::BufferHandle meshletDataBuffer;
    kage::BufferHandle meshDrawBuffer;
    kage::BufferHandle transformBuffer;

    kage::BufferHandle triPayloadBuffer;
    kage::BufferHandle triPayloadCountBuffer;

    
    kage::ImageHandle pyramid;
    kage::SamplerHandle pyrSampler;

    kage::BindlessHandle bindless;

    // read / write
    kage::ImageHandle depth;
    GBuffer g_buffer;

    // out-alias
    kage::ImageHandle depthOutAlias;
    GBuffer g_bufferOutAlias;
};

void initHardRaster(HardRaster& _hardRaster, const HardRasterInitData& _initData, const PassStage _stage);
void updateHardRaster(HardRaster& _hardRaster, const Constants& _consts);