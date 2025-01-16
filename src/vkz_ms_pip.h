#pragma once

#include "demo_structs.h"
#include "scene.h"
#include "kage.h"
#include "deferred/vkz_deferred.h"

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

    kage::ImageHandle depth;

    kage::BindlessHandle bindless;

    GBuffer g_buffer;
};

struct MeshShading
{
    bool late{ false };
    bool alphaPass{ false };

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

    kage::BindlessHandle bindless;
    
    // read / write
    kage::BufferHandle meshletVisBuffer;

    // read-only
    kage::ImageHandle pyramid;

    // write
    kage::ImageHandle depth;

    // deferred shading
    GBuffer g_buffer;

    // sampler
    kage::SamplerHandle pyramidSampler;

    // out aliases
    kage::BufferHandle meshletVisBufferOutAlias;
    kage::ImageHandle depthOutAlias;

    GBuffer g_bufferOutAlias;

    Globals globals;
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

void prepareMeshShading(MeshShading& _meshShading, const Scene& _scene, uint32_t _width, uint32_t _height, const MeshShadingInitData _initData, bool _late = false, bool _alphaPass = false);

void prepareTaskSubmit(TaskSubmit& _taskSubmit, kage::BufferHandle _drawCmdBuf, kage::BufferHandle _drawCmdCntBuf, bool _late = false, bool _alphaPass = false);


void updateTaskSubmit(const TaskSubmit& _taskSubmit);
void updateMeshShading(MeshShading& _meshShading, const Globals& _globals);