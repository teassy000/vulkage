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

    float cx, cy, cz;

    float       rayLength;
    float       probeSideLen;
    float       radius;

    float brx_tmin;
    float brx_tmax;

    uint32_t brx_offset;
    uint32_t brx_startCas;
    uint32_t brx_endCas;

    uint32_t debug_type;
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
void updateRadianceCascade(const RadianceCascade& _rc, const Dbg_RCBuild& _dbgRcBuild);

