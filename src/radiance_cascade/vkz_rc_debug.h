#pragma once
#include "kage_math.h"
#include "common.h"
#include "kage.h"
#include "kage_structs.h"
#include "demo_structs.h"

struct VoxDebugCmdGen
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle cs;

    kage::BufferHandle voxWorldPos;
    kage::BufferHandle voxAlbedo;
    kage::BufferHandle trans;
    kage::ImageHandle  pyramid;
    kage::BufferHandle cmdBuf;
    kage::BufferHandle drawBuf;
    kage::BufferHandle threadCountBuf;

    kage::SamplerHandle sampler;

    kage::BufferHandle outCmdAlias;
    kage::BufferHandle outDrawBufAlias;
};

struct VoxDebugDraw
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle vs;
    kage::ShaderHandle fs;

    kage::BufferHandle drawCmdBuf;
    kage::BufferHandle drawBuf;
    kage::BufferHandle trans;
    kage::ImageHandle color;
    kage::ImageHandle depth;

    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;

    kage::ImageHandle colorOutAlias;
    kage::ImageHandle depthOutAlias;
};

struct VoxDebug
{
    VoxDebugCmdGen cmdGen;
    VoxDebugDraw draw;
};

struct RCDebugInit
{
    kage::BufferHandle trans;
    kage::BufferHandle voxWPos;
    kage::BufferHandle voxAlbedo;
    kage::BufferHandle threadCount;

    kage::ImageHandle color;
    kage::ImageHandle depth;
    kage::ImageHandle cascade;
    kage::ImageHandle pyramid;
};

void prepareVoxDebug(VoxDebug& _voxDebug, const RCDebugInit& _init );
void updateVoxDebug(const VoxDebug& _vd, const DrawCull& _camCull, const uint32_t _widt, const uint32_t _height, const float _sceneRadius);

struct ProbeDbgCmdGen
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle cs;

    kage::BufferHandle trans;
    kage::BufferHandle cmdBuf;
    kage::BufferHandle drawDataBuf;

    kage::ImageHandle  pyramid;
    kage::SamplerHandle sampler;

    kage::BufferHandle outCmdAlias;
    kage::BufferHandle outDrawDataBufAlias;

    uint32_t idxCnt;
};

struct ProbeDbgDraw
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle vs;
    kage::ShaderHandle fs;

    kage::BufferHandle drawCmdBuf;
    kage::BufferHandle drawDataBuf;
    kage::BufferHandle trans;
    kage::ImageHandle color;
    kage::ImageHandle depth;
    kage::ImageHandle radianceCascade;
    kage::SamplerHandle rcSamp;

    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;

    kage::ImageHandle colorOutAlias;
    kage::ImageHandle depthOutAlias;
};

struct ProbeDebug
{
    ProbeDbgCmdGen cmdGen;
    ProbeDbgDraw draw;

    uint32_t debugLv;
};

void prepareProbeDebug(ProbeDebug& _pd, const RCDebugInit& _init);
void updateProbeDebug(const ProbeDebug& _pd, const DrawCull& _camCull, const uint32_t _width, const uint32_t _height, const float _sceneRadius);
