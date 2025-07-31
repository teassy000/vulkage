#pragma once

#include "core/kage.h"
#include "scene/scene.h"
#include "demo_structs.h"

struct VtxShadingInitData
{
    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle meshDrawCmdBuf;
    kage::BufferHandle transformBuf;
    kage::BufferHandle meshDrawCmdCountBuf;

    kage::ImageHandle color;
    kage::ImageHandle depth;

    kage::BindlessHandle bindless;
};


struct VtxShading
{
    bool late{ false };

    // read-only
    kage::PassHandle pass;
    kage::ShaderHandle vtxShader;
    kage::ShaderHandle fragShader;
    kage::ProgramHandle prog;

    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;

    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle meshDrawCmdBuf;
    kage::BufferHandle transformBuf;

    // read / write
    kage::BufferHandle meshDrawCmdCountBuf;

    // write
    kage::ImageHandle color;
    kage::ImageHandle depth;

    // out alias
    kage::BufferHandle meshDrawCmdBufOutAlias;
    kage::ImageHandle colorOutAlias;
    kage::ImageHandle depthOutAlias;

    // limits
    uint32_t maxMeshDrawCmdCount;

    // bindless
    kage::BindlessHandle bindless;

    Globals globals;
};


void prepareVtxShading(VtxShading& _vtxShading, const Scene& _scene, const VtxShadingInitData& _initData, bool _late = false);

void updateVtxShadingConstants(VtxShading& _vtxShading, const Globals& _globals);
