#pragma once
#include "core/kage.h"
#include "demo_structs.h"


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

    kage::BufferHandle cmdBufOutAlias;
    kage::BufferHandle cmdCountBufOutAlias;
    kage::BufferHandle meshDrawVisBufOutAlias;

    Constants constants;
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

    kage::ImageHandle pyramid;
    kage::SamplerHandle pyrSampler;
    
    // read / write
    kage::BufferHandle meshletVisBuf;

    // write
    kage::BufferHandle meshletPayloadBuf;
    kage::BufferHandle meshletPayloadCntBuf;

    // out alias
    kage::BufferHandle meshletVisBufOutAlias;
    kage::BufferHandle cmdBufOutAlias;
    kage::BufferHandle cmdCountBufOutAlias;
};

struct TriangleCullingInitData
{
    float screenWidth;
    float screenHeight;

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
    
    float screenWidth, screenHeight;

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
    kage::BufferHandle payloadCntBuf;

    // out alias
    kage::BufferHandle cmdBufOutAlias;
    kage::BufferHandle cmdCountBufOutAlias;
};

void initMeshCulling(MeshCulling& _mc, const MeshCullingInitData& _initData, RenderStage _stage, RenderPipeline _pass);

void updateMeshCulling(MeshCulling& _mc, const Constants& _consts, uint32_t _drawCount);


void initMeshletCulling(MeshletCulling& _mltc, const MeshletCullingInitData& _initData, RenderStage _stage, bool _seamless = false);
void updateMeshletCulling(MeshletCulling& _mltc, const Constants& _consts);

void initTriangleCulling(TriangleCulling& _tric, const TriangleCullingInitData& _initData, RenderStage _stage, bool _seamless = false);
void updateTriangleCulling(TriangleCulling& _tric, const Constants& _consts);

