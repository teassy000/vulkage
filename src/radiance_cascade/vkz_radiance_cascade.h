#pragma once

#include "kage_math.h"
#include "common.h"
#include "kage.h"
#include "deferred/vkz_deferred.h"
#include "demo_structs.h"
#include "ffx_intg/brixel_intg_kage.h"

// each page has a 3d grid of probes, each probe has a 2d grid of rays
struct alignas(16) RadianceCascadesConfig
{
    uint32_t    probe_sideCount;
    uint32_t    ray_gridSideCount;
    uint32_t    level;
    uint32_t    layerOffset;

    float       rayLength;
    float       probeSideLen;
    float       sceneRadius;
};

struct RadianceCascadeBuild
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle cs;

    GBuffer g_buffer;
    GBufferSamplers g_bufferSamplers;

    kage::BufferHandle trans;

    kage::ImageHandle inDepth;
    kage::SamplerHandle depthSampler;

    kage::ImageHandle inSkybox;
    kage::SamplerHandle skySampler;

    kage::ImageHandle cascadeImg;
    kage::ImageHandle radCascdOutAlias;

    BRX_UserResources brx;
    kage::SamplerHandle brxAtlasSamp;
};

struct RadianceCascade
{
    RadianceCascadeBuild rcBuild;
};

struct RadianceCascadeInitData
{
    GBuffer g_buffer;
    kage::ImageHandle depth;
    kage::ImageHandle skybox;
    kage::BufferHandle meshBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;
    kage::BufferHandle transBuf;
    kage::BindlessHandle bindless;

    // brx
    BRX_UserResources brx;

    uint32_t maxDrawCmdCount;
};


void prepareRadianceCascade(RadianceCascade& _rc, const RadianceCascadeInitData _init);
void updateRadianceCascade(
    const RadianceCascade& _rc
    , uint32_t _drawCount
    , const DrawCull& _camCull
    , const uint32_t _width
    , const uint32_t _height
    , const float _sceneRadiance
    );

