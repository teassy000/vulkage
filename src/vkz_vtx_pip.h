#pragma once

#include "vkz.h"
#include "scene.h"
#include "demo_structs.h"

struct VtxShadingInitData
{
    vkz::BufferHandle idxBuf;
    vkz::BufferHandle vtxBuf;
    vkz::BufferHandle meshDrawBuf;
    vkz::BufferHandle meshDrawCmdBuf;
    vkz::BufferHandle transformBuf;
    vkz::BufferHandle meshDrawCmdCountBuf;

    vkz::ImageHandle color;
    vkz::ImageHandle depth;
};


struct VtxShading
{
    // read-only
    vkz::ShaderHandle vtxShader;
    vkz::ShaderHandle fragShader;
    vkz::ProgramHandle prog;
    vkz::PassHandle pass;

    vkz::BufferHandle idxBuf;
    vkz::BufferHandle vtxBuf;

    vkz::BufferHandle meshDrawBuf;
    vkz::BufferHandle meshDrawCmdBuf;
    vkz::BufferHandle transformBuf;

    // read / write
    vkz::BufferHandle meshDrawCmdCountBuf;

    // write
    vkz::ImageHandle color;
    vkz::ImageHandle depth;

    // out alias
    vkz::BufferHandle meshDrawCmdBufOutAlias;
    vkz::ImageHandle colorOutAlias;
    vkz::ImageHandle depthOutAlias;

    GlobalsVKZ globals;
};


void prepareVtxShading(VtxShading& _vtxShading, const Scene& _scene, const VtxShadingInitData& _initData, bool _late = false);

void updateVtxShadingConstants(VtxShading& _vtxShading, const GlobalsVKZ& _globals);
