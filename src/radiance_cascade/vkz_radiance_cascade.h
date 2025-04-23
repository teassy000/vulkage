#pragma once

#include "kage_math.h"
#include "common.h"
#include "kage.h"
#include "deferred/vkz_deferred.h"
#include "demo_structs.h"
#include "ffx_intg/brixel_intg_kage.h"

enum class RCDbgIndexType : uint32_t
{
    probe_first = 0,
    ray_first,
    count
};

enum class RCDbgColorType : uint32_t
{
    albedo = 0,
    normal,
    worldPos,
    emissive,
    brxDistance,
    brxNormal,
    rayDir,
    probePos,
    probeHash,
    count
};

// each page has a 3d grid of probes, each probe has a 2d grid of rays
struct alignas(16) RadianceCascadesConfig
{
    uint32_t    probe_sideCount;
    uint32_t    ray_gridSideCount;
    uint32_t    level;
    uint32_t    layerOffset;

    float cx, cy, cz;

    float       rayEndLength;
    float       rayStartLength;
    float       probeSideLen;
    float       radius;

    float brx_tmin;
    float brx_tmax;

    uint32_t brx_offset;
    uint32_t brx_startCas;
    uint32_t brx_endCas;
    float brx_sdfEps;

    uint32_t debug_idx_type;
    uint32_t debug_color_type;
};

struct alignas(16) RadianceCascadesTransform
{
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
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

    vec3 cameraPos{vec3(0.f)};
    mat4 view{ mat4(1.f) };
    mat4 proj{ mat4(1.f) };
};

struct RadianceCascadeMerge
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle cs;

    kage::ImageHandle radianceCascade;
    kage::SamplerHandle rcSampler;

    kage::SamplerHandle mergedSampler;

    kage::ImageHandle skybox;
    kage::SamplerHandle skySampler;
    
    kage::ImageHandle mergedCascade;
    kage::ImageHandle mergedCascadesAlias;

    uint32_t currLv;
    bool    rayPrime;
};

struct RadianceCascade
{
    RadianceCascadeBuild build;
    RadianceCascadeMerge mergeRay;
    RadianceCascadeMerge mergeProbe;
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
    kage::BindlessHandle bindless;

    // brx
    BRX_UserResources brx;

    uint32_t maxDrawCmdCount;
    uint32_t currCas;
};


void prepareRadianceCascade(RadianceCascade& _rc, const RadianceCascadeInitData _init);
void updateRadianceCascade(RadianceCascade& _rc, const Dbg_RadianceCascades& _dbgRcBuild, const TransformData& _trans);

