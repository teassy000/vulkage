#pragma once
#include "kage_math.h"
#include "common.h"
#include "kage.h"
#include "kage_structs.h"
#include "demo_structs.h"

struct RCDebugInit
{
    kage::BufferHandle trans;

    kage::ImageHandle color;
    kage::ImageHandle depth;
    kage::ImageHandle cascade;
    kage::ImageHandle pyramid;
};

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
};

void prepareProbeDebug(ProbeDebug& _pd, const RCDebugInit& _init);
void updateProbeDebug(const ProbeDebug& _pd, const DrawCull& _camCull, const uint32_t _width, const uint32_t _height, const Dbg_RadianceCascades _rcDbg);
