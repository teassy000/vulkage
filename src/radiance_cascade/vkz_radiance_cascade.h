#pragma once

#include "kage_math.h"
#include "common.h"
#include "kage.h"
#include "deferred/vkz_deferred.h"

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
    float       ot_sceneSideLen;
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

    kage::BufferHandle fragCountBuf;

    kage::BufferHandle voxMap;
    kage::BufferHandle voxelWorldPos;
    kage::BufferHandle voxelAlbedo;
    kage::BufferHandle voxelNormal;

    kage::ImageHandle rt;

    kage::BufferHandle fragCountBufOutAlias;

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

struct VoxDebugDrawCmdGen
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle cs;

    kage::BufferHandle voxmap;
    kage::BufferHandle trans;
    kage::ImageHandle  pyramid;
    kage::BufferHandle cmdBuf;
    kage::BufferHandle cmdCountBuf;

    kage::SamplerHandle sampler;

    kage::BufferHandle outCmdAlias;
    kage::BufferHandle outCmdCountAlias;
};

struct VoxDebug
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle vs;
    kage::ShaderHandle fs;

    kage::BufferHandle drawCmdBuf;
    kage::BufferHandle drawCmdCountBuf;
    kage::BufferHandle trans;
    kage::ImageHandle renderTarget;

    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;

    uint32_t width;
    uint32_t height;

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
    kage::ImageHandle outAlias;

    float sceneRadius;
    RadianceCascadesConfig lv0Config;
};

struct RadianceCascade
{
    VoxelizationCmd voxCmd;
    Voxelization vox;
    OctTree octTree;
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
void updateRadianceCascade(const RadianceCascade& _rc, uint32_t _drawCount, const mat4& _proj);
