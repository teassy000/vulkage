#pragma once
#include "core/kage.h"
#include "demo_structs.h"


enum class CullingStage : uint8_t
{
    early = 0,      // early culling pass
    late = 1,       // late culling pass
    alpha = 2,      // alpha culling pass
    count = 3,      // count of culling stages
};

enum class CullingPass : uint8_t
{
    normal = 0,     // for normal pipeline, i.e. traditional rasterization
    task = 1,       // for mesh shading task
    compute = 2,    // for mix rasterization
    count = 3, // count of culling passes
};

struct MeshCullingInitData
{
    kage::BufferHandle meshBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transBuf;

    kage::ImageHandle pyramid;

    kage::BufferHandle meshDrawCmdBuf;
    kage::BufferHandle meshDrawCmdCountBuf;
    kage::BufferHandle meshDrawVisBuf;
};

struct MeshCulling
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

struct MeshletCullingInitData
{
    kage::BufferHandle meshBuf;
    kage::BufferHandle meshletBuf;
    kage::BufferHandle meshletDataBuf;
    kage::BufferHandle meshDrawCmdBuf;
    kage::BufferHandle meshletDrawCmdCntBuf;
    kage::BufferHandle meshletVisBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transformBuf;
    kage::ImageHandle pyramid;
};

struct MeshletCulling
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;
    // read-only
    kage::BufferHandle meshBuf;
    kage::BufferHandle meshletBuf;
    kage::BufferHandle meshletDataBuf;
    kage::BufferHandle meshDrawCmdBuf;
    kage::BufferHandle meshletDrawCmdCntBuf;
    kage::BufferHandle meshletVisBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transformBuf;
    kage::ImageHandle pyramid;
    kage::SamplerHandle pyrSampler;
    // output
    kage::BufferHandle meshletDrawCmdCntBufOutAlias;
    kage::BufferHandle meshletVisBufOutAlias;
};

struct TriangleCullingInitData
{
    kage::BufferHandle inBuf;
};

struct TriangleCulling
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;
    // read-only
    kage::BufferHandle inBuf;
};

void prepareMeshCulling(MeshCulling& _cullingComp, const MeshCullingInitData& _initData, CullingStage _stage, CullingPass _pass);

void updateMeshCulling(MeshCulling& _cullingComp, const DrawCull& _drawCull, uint32_t _drawCount);


void initMeshletCulling(MeshletCulling& _cullingComp, const MeshletCullingInitData& _initData);
void updateMeshletCulling(MeshletCulling& _cullingComp, const DrawCull& _drawCull, uint32_t _drawCount);

void initTriangleCulling(TriangleCulling& _cullingComp, const TriangleCullingInitData& _initData);
void updateTriangleCulling(TriangleCulling& _cullingComp, const DrawCull& _drawCull, uint32_t _drawCount);

