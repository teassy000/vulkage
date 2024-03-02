#pragma once

#include "demo_structs.h"
#include "scene.h"
#include "vkz.h"

struct MeshShadingInitData
{
    vkz::BufferHandle vtxBuffer;
    vkz::BufferHandle meshBuffer;
    vkz::BufferHandle meshletBuffer;
    vkz::BufferHandle meshletDataBuffer;
    vkz::BufferHandle meshDrawBuffer;

    vkz::BufferHandle meshDrawCmdBuffer;
    vkz::BufferHandle meshDrawCmdCountBuffer;
    vkz::BufferHandle meshletVisBuffer;

    vkz::BufferHandle transformBuffer;

    vkz::ImageHandle pyramid;

    vkz::ImageHandle color;
    vkz::ImageHandle depth;
};

struct MeshShading
{
    vkz::ShaderHandle taskShader;
    vkz::ShaderHandle meshShader;
    vkz::ShaderHandle fragShader;

    vkz::ProgramHandle program;
    vkz::PassHandle pass;

    // read-only
    vkz::BufferHandle vtxBuffer;
    vkz::BufferHandle meshBuffer;
    vkz::BufferHandle meshletBuffer;
    vkz::BufferHandle meshletDataBuffer;
    vkz::BufferHandle meshDrawCmdBuffer;
    vkz::BufferHandle meshDrawCmdCountBuffer;
    vkz::BufferHandle meshDrawBuffer;
    vkz::BufferHandle transformBuffer;
    
    // read / write
    vkz::BufferHandle meshletVisBuffer;

    // read-only
    vkz::ImageHandle pyramid;

    // write
    vkz::ImageHandle color;
    vkz::ImageHandle depth;

    // sampler
    vkz::SamplerHandle pyramidSampler;

    // out aliases
    vkz::BufferHandle meshletVisBufferOutAlias;
    vkz::ImageHandle colorOutAlias;
    vkz::ImageHandle depthOutAlias;

    uint32_t width;
    uint32_t height;

    GlobalsVKZ globals;
};

struct MeshShadingConfig
{
    bool late{ false };
    bool task{ false };
};

struct TaskSubmit
{
    vkz::ShaderHandle cs;
    vkz::ProgramHandle prog;

    vkz::PassHandle pass;

    vkz::BufferHandle drawCmdBuffer;
    vkz::BufferHandle drawCmdCountBuffer;
    vkz::BufferHandle drawCmdBufferOutAlias;
};

void prepareMeshShading(MeshShading& _meshShading, const Scene& _scene, uint32_t _width, uint32_t _height, const MeshShadingInitData _initData, bool _late = false);

void prepareTaskSubmit(TaskSubmit& _taskSubmit, vkz::BufferHandle _drawCmdBuf, vkz::BufferHandle _drawCmdCntBuf, bool _late = false);

void updateMeshShadingConstants(MeshShading& _meshShading, const GlobalsVKZ& _globals);