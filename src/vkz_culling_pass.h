#pragma once
#include "vkz.h"
#include "demo_structs.h"

struct CullingCompInitData
{
    vkz::BufferHandle meshBuf;
    vkz::BufferHandle meshDrawBuf;
    vkz::BufferHandle transBuf;

    vkz::ImageHandle pyramid;

    vkz::BufferHandle meshDrawCmdBuf;
    vkz::BufferHandle meshDrawCmdCountBuf;
    vkz::BufferHandle meshDrawVisBuf;
};

struct CullingComp
{
    vkz::ShaderHandle cs;
    vkz::ProgramHandle prog;
    vkz::PassHandle pass;

    // read-only
    vkz::BufferHandle meshBuf;
    vkz::BufferHandle meshDrawBuf;
    vkz::BufferHandle transBuf;

    vkz::ImageHandle pyramid;

    // read / write
    vkz::BufferHandle meshDrawCmdBuf;
    vkz::BufferHandle meshDrawCmdCountBuf;
    vkz::BufferHandle meshDrawVisBuf;

    vkz::BufferHandle meshDrawCmdBufOutAlias;
    vkz::BufferHandle meshDrawCmdCountBufOutAlias;
    vkz::BufferHandle meshDrawVisBufOutAlias;

    MeshDrawCullVKZ meshDrawCull;
};

void prepareCullingComp(CullingComp& _cullingComp, const CullingCompInitData& _initData, bool _late = false, bool _task = false);

void updateCullingConstants(CullingComp& _cullingComp, const MeshDrawCullVKZ& _drawCull);

