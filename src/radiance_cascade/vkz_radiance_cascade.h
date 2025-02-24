#pragma once

#include "kage_math.h"
#include "common.h"
#include "kage.h"
#include "deferred/vkz_deferred.h"
#include "demo_structs.h"

// each page has a 3d grid of probes, each probe has a 2d grid of rays
struct alignas(16) RadianceCascadesConfig
{
    uint32_t    probe_sideCount;
    uint32_t    ray_gridSideCount;
    uint32_t    level;
    uint32_t    layerOffset;

    float       rayLength;
    float       probeSideLen;
    
    // oct-tree dat
    uint32_t    ot_voxSideCount;
    float       ot_sceneRadius;
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

    kage::BufferHandle threadCountBuf;

    kage::BufferHandle voxMap;
    kage::BufferHandle voxelWorldPos;
    kage::BufferHandle voxelAlbedo;
    kage::BufferHandle voxelNormal;

    kage::BindlessHandle bindless;

    kage::ImageHandle rt;

    kage::BufferHandle threadCountBufOutAlias;

    kage::BufferHandle voxMapOutAlias;
    kage::BufferHandle wposOutAlias;
    kage::BufferHandle albedoOutAlias;
    kage::BufferHandle normalOutAlias;

    uint32_t maxDrawCmdCount;
};

struct OctTree
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle cs;

    kage::BufferHandle inVoxMap;
    kage::BufferHandle voxMediemMap;
    kage::BufferHandle outOctTree;
    kage::BufferHandle nodeCount;

    kage::BufferHandle nodeCountOutAlias;
    kage::BufferHandle octTreeOutAlias;
};

struct VoxDebugCmdGen
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle cs;

    kage::BufferHandle voxmap;
    kage::BufferHandle voxWorldPos;
    kage::BufferHandle trans;
    kage::ImageHandle  pyramid;
    kage::BufferHandle cmdBuf;
    kage::BufferHandle drawBuf;
    kage::BufferHandle threadCountBuf;

    kage::SamplerHandle sampler;

    kage::BufferHandle outCmdAlias;
    kage::BufferHandle outDrawBufAlias;
};

struct VoxDebug
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle vs;
    kage::ShaderHandle fs;

    kage::BufferHandle drawCmdBuf;
    kage::BufferHandle drawBuf;
    kage::BufferHandle trans;
    kage::ImageHandle renderTarget;

    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;


    kage::ImageHandle rtOutAlias;
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

    kage::BufferHandle inOctTreeNodeCount;
    kage::BufferHandle inOctTree;

    kage::BufferHandle voxAlbedo;

    kage::ImageHandle cascadeImg;
    kage::ImageHandle radCascdOutAlias;

    RadianceCascadesConfig lv0Config;
};

struct RadianceCascade
{
    VoxelizationCmd voxCmd;
    Voxelization vox;
    OctTree octTree;
    RadianceCascadeBuild rcBuild;

    VoxDebugCmdGen voxDebugCmdGen;
    VoxDebug voxDebug;
};

struct RadianceCascadeInitData
{
    GBuffer g_buffer;
    kage::ImageHandle color;
    kage::ImageHandle depth;
    kage::ImageHandle pyramid;
    kage::BufferHandle meshBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;
    kage::BufferHandle transBuf;
    kage::BindlessHandle bindless;

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

