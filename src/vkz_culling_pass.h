#pragma once
#include "kage.h"
#include "demo_structs.h"

struct CullingCompInitData
{
    kage::BufferHandle meshBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transBuf;

    kage::ImageHandle pyramid;

    kage::BufferHandle meshDrawCmdBuf;
    kage::BufferHandle meshDrawCmdCountBuf;
    kage::BufferHandle meshDrawVisBuf;
};

struct Culling
{
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;
    kage::PassHandle pass;

    // read-only
    kage::BufferHandle meshBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transBuf;

    kage::ImageHandle pyramid;
    kage::SamplerHandle pyrSampler;

    // read / write
    kage::BufferHandle meshDrawCmdBuf;
    kage::BufferHandle meshDrawCmdCountBuf;
    kage::BufferHandle meshDrawVisBuf;

    kage::BufferHandle meshDrawCmdBufOutAlias;
    kage::BufferHandle meshDrawCmdCountBufOutAlias;
    kage::BufferHandle meshDrawVisBufOutAlias;

    DrawCull drawCull;
};

void prepareCullingComp(Culling& _cullingComp, const CullingCompInitData& _initData, bool _late = false, bool _task = false, bool _alphaPass = false);

void updateCulling(Culling& _cullingComp, const DrawCull& _drawCull, uint32_t _drawCount);

