#pragma once

#include "kage_math.h"
#include "common.h"
#include "kage.h"
#include "deferred/vkz_deferred.h"

struct alignas(16) RadianceCascadesConfig
{
    uint32_t rayGridDiameter;
    uint32_t probeDiameter;
    uint32_t level;
    uint32_t layerOffset;

    float rayLength;
    uint32_t rayMarchingSteps;
};

struct VoxelizationCmd
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle program;

    kage::BufferHandle in_meshBuf;
    kage::BufferHandle in_drawBuf;
    kage::BufferHandle out_cmdBuf;
    kage::BufferHandle out_cmdCountBuf;

    kage::BufferHandle out_cmdBufAlias;
    kage::BufferHandle out_cmdCountBufAlias;
};

struct Voxelization
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle vs;
    kage::ShaderHandle gs;
    kage::ShaderHandle fs;

    kage::BufferHandle cmdBuf;
    kage::BufferHandle cmdCountBuf;
    kage::BufferHandle drawBuf;
    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;
    kage::BufferHandle transBuf;

    kage::ImageHandle voxelAlbedo;
    kage::ImageHandle voxelNormal;
    kage::ImageHandle voxelWorldPos;

    kage::ImageHandle rt;

    kage::ImageHandle voxelAlbedoOutAlias;

    uint32_t maxDrawCmdCount;
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

    kage::ImageHandle voxAlbedo;
    kage::SamplerHandle voxAlbedoSampler;

    kage::ImageHandle cascadeImg;
    kage::ImageHandle outAlias;

    RadianceCascadesConfig lv0Config;
};

struct RadianceCascade
{
    VoxelizationCmd voxCmd;
    Voxelization vox;
    RadianceCascadeBuild rcBuild;
};

struct RadianceCascadeInitData
{
    GBuffer g_buffer;
    kage::ImageHandle depth;
    kage::BufferHandle meshBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;
    kage::BufferHandle transBuf;

    float sceneRadius;
    uint32_t maxDrawCmdCount;
};

void prepareRadianceCascade(RadianceCascade& _rc, const RadianceCascadeInitData _init);
void updateRadianceCascade(const RadianceCascade& _rc);
