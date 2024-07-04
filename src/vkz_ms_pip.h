#pragma once

#include "demo_structs.h"
#include "scene.h"
#include "kage.h"

struct MeshShadingInitData
{
    kage::BufferHandle vtxBuffer;
    kage::BufferHandle meshBuffer;
    kage::BufferHandle meshletBuffer;
    kage::BufferHandle meshletDataBuffer;
    kage::BufferHandle meshDrawBuffer;

    kage::BufferHandle meshDrawCmdBuffer;
    kage::BufferHandle meshDrawCmdCountBuffer;
    kage::BufferHandle meshletVisBuffer;

    kage::BufferHandle transformBuffer;

    kage::ImageHandle pyramid;

    kage::ImageHandle color;
    kage::ImageHandle depth;
};

struct MeshShading
{
    kage::PassHandle pass;

    kage::ShaderHandle taskShader;
    kage::ShaderHandle meshShader;
    kage::ShaderHandle fragShader;

    kage::ProgramHandle program;

    // read-only
    kage::BufferHandle vtxBuffer;
    kage::BufferHandle meshBuffer;
    kage::BufferHandle meshletBuffer;
    kage::BufferHandle meshletDataBuffer;
    kage::BufferHandle meshDrawCmdBuffer;
    kage::BufferHandle meshDrawCmdCountBuffer;
    kage::BufferHandle meshDrawBuffer;
    kage::BufferHandle transformBuffer;
    
    // read / write
    kage::BufferHandle meshletVisBuffer;

    // read-only
    kage::ImageHandle pyramid;

    // write
    kage::ImageHandle color;
    kage::ImageHandle depth;

    // sampler
    kage::SamplerHandle pyramidSampler;

    // out aliases
    kage::BufferHandle meshletVisBufferOutAlias;
    kage::ImageHandle colorOutAlias;
    kage::ImageHandle depthOutAlias;

    Globals globals;
};

struct MeshShadingConfig
{
    bool late{ false };
    bool task{ false };
};

struct TaskSubmit
{
    kage::PassHandle pass;

    kage::ShaderHandle cs;
    kage::ProgramHandle prog;

    kage::BufferHandle drawCmdBuffer;
    kage::BufferHandle drawCmdCountBuffer;
    kage::BufferHandle drawCmdBufferOutAlias;
};

void prepareMeshShading(MeshShading& _meshShading, const Scene& _scene, uint32_t _width, uint32_t _height, const MeshShadingInitData _initData, bool _late = false);

void prepareTaskSubmit(TaskSubmit& _taskSubmit, kage::BufferHandle _drawCmdBuf, kage::BufferHandle _drawCmdCntBuf, bool _late = false);


void updateTaskSubmit(const TaskSubmit& _taskSubmit);
void updateMeshShading(MeshShading& _meshShading, const Globals& _globals);