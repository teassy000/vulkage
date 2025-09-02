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
    kage::BufferHandle meshletCmdBuf;
    kage::BufferHandle meshletCmdCntBuf;
    kage::BufferHandle meshBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transformBuf;
    kage::BufferHandle meshletBuf;
    kage::BufferHandle meshletVisBuf;

    kage::ImageHandle pyramid;
};

struct MeshletCulling
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;
    
    // read-only
    kage::BufferHandle meshletCmdBuf;
    kage::BufferHandle meshletCmdCntBuf; // for indirect dispatch
    kage::BufferHandle meshBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transformBuf;
    kage::BufferHandle meshletBuf;
    kage::BufferHandle meshletVisBuf;
    kage::ImageHandle pyramid;
    kage::SamplerHandle pyrSampler;
    
    // write
    kage::BufferHandle meshletPayloadBuf;
    kage::BufferHandle meshletPayloadCntBuf;

    // out alias
    kage::BufferHandle meshletPayloadBufOutAlias;
    kage::BufferHandle meshletPayloadCntOutAlias;
};

struct TriangleCullingInitData
{
    kage::BufferHandle meshletPayloadBuf;
    kage::BufferHandle meshletPayloadCntBuf;

    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transformBuf;
    kage::BufferHandle vtxBuf;
    kage::BufferHandle meshletBuf;
    kage::BufferHandle meshletDataBuf;
};

struct TriangleCulling
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;
    
    // read-only
    kage::BufferHandle meshletPayloadBuf;
    kage::BufferHandle meshletPayloadCntBuf;
    kage::BufferHandle meshDrawBuf;
    kage::BufferHandle transformBuf;
    kage::BufferHandle vtxBuf;
    kage::BufferHandle meshletBuf;
    kage::BufferHandle meshletDataBuf;

    // write
    kage::BufferHandle trianglePayloadBuf;
    kage::BufferHandle trianglePayloadCntBuf;

    // out alias
    kage::BufferHandle trianglePayloadBufOutAlias;
    kage::BufferHandle trianglePayloadCntOutAlias;
};

void initMeshCulling(MeshCulling& _mc, const MeshCullingInitData& _initData, CullingStage _stage, CullingPass _pass);

void updateMeshCulling(MeshCulling& _mc, const DrawCull& _drawCull, uint32_t _drawCount);


void initMeshletCulling(MeshletCulling& _mltc, const MeshletCullingInitData& _initData, CullingStage _stage, bool _seamless = false);
void updateMeshletCulling(MeshletCulling& _mltc, const DrawCull& _drawCull);

void initTriangleCulling(TriangleCulling& _tric, const TriangleCullingInitData& _initData, CullingStage _stage, bool _seamless = false);
void updateTriangleCulling(TriangleCulling& _tric, const DrawCull& _drawCull);

