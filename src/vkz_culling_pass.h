#pragma once
#include "vkz.h"
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

struct CullingComp
{
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;
    kage::PassHandle pass;

    // read-only
    kage::BufferHandle meshBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transBuf;

    kage::ImageHandle pyramid;

    // read / write
    kage::BufferHandle meshDrawCmdBuf;
    kage::BufferHandle meshDrawCmdCountBuf;
    kage::BufferHandle meshDrawVisBuf;

    kage::BufferHandle meshDrawCmdBufOutAlias;
    kage::BufferHandle meshDrawCmdCountBufOutAlias;
    kage::BufferHandle meshDrawVisBufOutAlias;

    MeshDrawCullVKZ meshDrawCull;
};

void prepareCullingComp(CullingComp& _cullingComp, const CullingCompInitData& _initData, bool _late = false, bool _task = false);

void updateCullingConstants(CullingComp& _cullingComp, const MeshDrawCullVKZ& _drawCull);

