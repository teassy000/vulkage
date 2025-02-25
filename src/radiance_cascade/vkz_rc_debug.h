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

struct VoxDebugDraw
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

struct VoxDebug
{
    VoxDebugCmdGen cmdGen;
    VoxDebugDraw draw;
};

struct VoxDebugInit
{
    kage::ImageHandle pyramid;
    kage::ImageHandle rt;
    kage::BufferHandle trans;
    kage::BufferHandle voxMap;
    kage::BufferHandle voxWPos;
    kage::BufferHandle threadCount;
};

void prepareVoxDebug(VoxDebug& _voxDebug, const VoxDebugInit& _init );
void updateVoxDebug(const VoxDebug& _vd, const DrawCull& _camCull, const uint32_t _widt, const uint32_t _height, const float _sceneRadius);