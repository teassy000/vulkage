#include "vkz_culling_pass.h"
#include "vkz_pass.h"

void recMeshCulling(const MeshCulling& _cull, uint32_t _drawCount)
{
    KG_ZoneScopedC(kage::Color::blue);

    const kage::Memory* mem = kage::alloc(sizeof(Constants));
    bx::memCopy(mem->data, &_cull.constants, mem->size);

    kage::startRec(_cull.pass);

    kage::fillBuffer(_cull.meshDrawCmdCountBuf, 0);

    kage::setConstants(mem);

    kage::Binding binds[] =
    {
        { _cull.meshBuf,             BindingAccess::read,       Stage::compute_shader },
        { _cull.meshDrawBuf,         BindingAccess::read,       Stage::compute_shader },
        { _cull.transBuf,            BindingAccess::read,       Stage::compute_shader },
        { _cull.meshDrawCmdBuf,      BindingAccess::write,      Stage::compute_shader },
        { _cull.meshDrawCmdCountBuf, BindingAccess::read_write, Stage::compute_shader },
        { _cull.meshDrawVisBuf,      BindingAccess::read_write, Stage::compute_shader },
        { _cull.pyramid,             _cull.pyrSampler,          Stage::compute_shader },
    };
    kage::pushBindings(binds, COUNTOF(binds));
    kage::dispatch(_drawCount, 1, 1);

    kage::endRec();
}

void initMeshCulling(MeshCulling& _cullingComp, const MeshCullingInitData& _initData, PassStage _stage, RenderPipeline _pipeline)
{
    kage::ShaderHandle cs = kage::registShader("mesh_draw_cmd", "shader/drawcmd.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("mesh_draw_cmd", { cs }, sizeof(Constants));

    int pipelineSpecs[] = { 
        _stage == PassStage::late // LATE
        , _pipeline == RenderPipeline::mesh_shading // TASK
        , _stage == PassStage::alpha // ALPHA_PASS
        , _pipeline == RenderPipeline::mixed // USE_MIXED_RASTER
    };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc;
    passDesc.prog = prog;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    passDesc.pipelineSpecData = (void*)pConst->data;

    std::string passName = getPassName( "mesh_culling", _stage, _pipeline);
    kage::PassHandle pass = kage::registPass(passName.c_str(), passDesc);

    kage::BufferHandle drawCmdOutAlias = kage::alias(_initData.meshDrawCmdBuf);
    kage::BufferHandle drawCmdCountOutAlias = kage::alias(_initData.meshDrawCmdCountBuf);
    kage::BufferHandle drawVisOutAlias = kage::alias(_initData.meshDrawVisBuf);

    kage::bindBuffer(pass, _initData.meshBuf
        , Stage::compute_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _initData.meshDrawBuf
        , Stage::compute_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _initData.transBuf
        , Stage::compute_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _initData.meshDrawCmdBuf
        , Stage::compute_shader
        , Access::shader_read | kage::AccessFlagBits::shader_write
        , drawCmdOutAlias);

    kage::bindBuffer(pass, _initData.meshDrawCmdCountBuf
        , Stage::compute_shader
        , Access::shader_read | kage::AccessFlagBits::shader_write
        , drawCmdCountOutAlias);

    kage::bindBuffer(pass, _initData.meshDrawVisBuf
        , Stage::compute_shader
        , Access::shader_read | kage::AccessFlagBits::shader_write
        , drawVisOutAlias);

     kage::SamplerHandle samp = kage::sampleImage(pass, _initData.pyramid
        , Stage::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
     );

    _cullingComp.cs = cs;
    _cullingComp.prog = prog;
    _cullingComp.pass = pass;

    _cullingComp.meshBuf = _initData.meshBuf;
    _cullingComp.meshDrawBuf = _initData.meshDrawBuf;
    _cullingComp.transBuf = _initData.transBuf;
    _cullingComp.meshDrawCmdBuf = _initData.meshDrawCmdBuf;
    _cullingComp.meshDrawCmdCountBuf = _initData.meshDrawCmdCountBuf;
    _cullingComp.meshDrawVisBuf = _initData.meshDrawVisBuf;
    _cullingComp.pyramid = _initData.pyramid;
    _cullingComp.pyrSampler = samp;

    _cullingComp.cmdBufOutAlias = drawCmdOutAlias;
    _cullingComp.cmdCountBufOutAlias = drawCmdCountOutAlias;
    _cullingComp.meshDrawVisBufOutAlias = drawVisOutAlias;

    _cullingComp.constants = {};
}

void updateMeshCulling(MeshCulling& _cullingComp, const Constants& _consts, uint32_t _drawCount)
{
    _cullingComp.constants = _consts;

    recMeshCulling(_cullingComp, _drawCount);
}

void initMeshletCulling(MeshletCulling& _cullingComp, const MeshletCullingInitData& _initData, PassStage _stage, bool _seamless /*= false*/)
{
    kage::ShaderHandle cs = kage::registShader("meshlet_culling", "shader/culling_meshlet.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("meshlet_culling", { cs }, sizeof(Constants));

    int pipelineSpecs[] = {
        _stage == PassStage::late
        , _stage == PassStage::alpha
        , _seamless
    };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc;
    passDesc.prog = prog;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    passDesc.pipelineSpecData = (void*)pConst->data;

    std::string passName = getPassName("meshlet_culling", _stage);
    kage::PassHandle pass = kage::registPass(passName.c_str(), passDesc);

    kage::BufferDesc meshletPayloadBufDesc;
    meshletPayloadBufDesc.size = 128 * 1024 * 1024; // 128M
    meshletPayloadBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    meshletPayloadBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle meshletPayloadBuf = kage::registBuffer("meshlet_payload", meshletPayloadBufDesc);

    kage::BufferDesc  meshletPayloadCntDesc;
    meshletPayloadCntDesc.size = 16; // the x field is the draw count, then cmd resolution
    meshletPayloadCntDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
    meshletPayloadCntDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle meshletPayloadCntBuf = kage::registBuffer("meshlet_payload_cnt", meshletPayloadCntDesc);

    kage::BufferHandle meshletVisBufOutAlias = kage::alias(_initData.meshletVisBuf);
    kage::BufferHandle meshletPayloadBufOutAlias = kage::alias(meshletPayloadBuf);
    kage::BufferHandle meshletPayloadCntOutAlias = kage::alias(meshletPayloadCntBuf);

    kage::bindBuffer(pass
        , _initData.meshletCmdBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::setIndirectBuffer(pass, _initData.meshletCmdCntBuf);

    kage::bindBuffer(pass
        , _initData.meshBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        , _initData.meshDrawBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        , _initData.transformBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        , _initData.meshletBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        , _initData.meshletCmdCntBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    // read/write buffers
    kage::bindBuffer(pass
        , _initData.meshletVisBuf
        , Stage::compute_shader
        , Access::shader_read| kage::AccessFlagBits::shader_write
        , meshletVisBufOutAlias
    );

    // write buffers
    kage::bindBuffer(pass
        , meshletPayloadBuf
        , Stage::compute_shader
        , Access::shader_read | kage::AccessFlagBits::shader_write
        , meshletPayloadBufOutAlias
    );

    kage::bindBuffer(pass
        , meshletPayloadCntBuf
        , Stage::compute_shader
        , Access::shader_read | kage::AccessFlagBits::shader_write
        , meshletPayloadCntOutAlias
    );

    // samplers
    kage::SamplerHandle pyrSamp = kage::sampleImage( pass
        , _initData.pyramid
        , Stage::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    _cullingComp.cs = cs;
    _cullingComp.prog = prog;
    _cullingComp.pass = pass;

    // read-only
    _cullingComp.meshletCmdBuf = _initData.meshletCmdBuf;
    _cullingComp.meshletCmdCntBuf = _initData.meshletCmdCntBuf;
    _cullingComp.meshBuf = _initData.meshBuf;
    _cullingComp.meshDrawBuf = _initData.meshDrawBuf;
    _cullingComp.transformBuf = _initData.transformBuf;
    _cullingComp.meshletBuf = _initData.meshletBuf;
    _cullingComp.pyramid = _initData.pyramid;
    _cullingComp.pyrSampler = pyrSamp;

    // read-write
    _cullingComp.meshletVisBuf = _initData.meshletVisBuf;
    _cullingComp.meshletPayloadBuf = meshletPayloadBuf;
    _cullingComp.meshletPayloadCntBuf = meshletPayloadCntBuf;

    // out-alias
    _cullingComp.meshletVisBufOutAlias = meshletVisBufOutAlias;
    _cullingComp.cmdBufOutAlias = meshletPayloadBufOutAlias;
    _cullingComp.cmdCountBufOutAlias = meshletPayloadCntOutAlias;
}

void recMeshletCulling(const MeshletCulling& _mltc, const Constants& _consts)
{
    KG_ZoneScopedC(kage::Color::blue);

    const kage::Memory* mem = kage::alloc(sizeof(Constants));
    bx::memCopy(mem->data, &_consts, mem->size);

    kage::startRec(_mltc.pass);

    kage::fillBuffer(_mltc.meshletPayloadCntBuf, 0);

    kage::setConstants(mem);

    kage::Binding binds[] =
    {
        { _mltc.meshletCmdBuf,          BindingAccess::read,        Stage::compute_shader },
        { _mltc.meshBuf,                BindingAccess::read,        Stage::compute_shader },
        { _mltc.meshDrawBuf,            BindingAccess::read,        Stage::compute_shader },
        { _mltc.transformBuf,           BindingAccess::read,        Stage::compute_shader },
        { _mltc.meshletBuf,             BindingAccess::read,        Stage::compute_shader },
        { _mltc.meshletCmdCntBuf,       BindingAccess::read,        Stage::compute_shader },
        { _mltc.meshletVisBuf,          BindingAccess::read_write,  Stage::compute_shader },
        { _mltc.meshletPayloadBuf,      BindingAccess::write,       Stage::compute_shader },
        { _mltc.meshletPayloadCntBuf,   BindingAccess::write,       Stage::compute_shader },
        { _mltc.pyramid,                _mltc.pyrSampler,           Stage::compute_shader },
    };
    kage::pushBindings(binds, COUNTOF(binds));

    kage::dispatchIndirect(_mltc.meshletCmdCntBuf, offsetof(IndirectDispatchCommand, x));
    kage::endRec();
}

void updateMeshletCulling(MeshletCulling& _mltc, const Constants& _consts)
{
    recMeshletCulling(_mltc, _consts);
}

void initTriangleCulling(TriangleCulling& _tric, const TriangleCullingInitData& _initData, PassStage _stage, bool _seamless /*= false*/)
{
    kage::ShaderHandle cs = kage::registShader("triangle_culling", "shader/culling_triangle.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("triangle_culling", { cs }, sizeof(Constants));

    int pipelineSpecs[] = {
        _stage == PassStage::late
        , _stage == PassStage::alpha
        , _seamless
    };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc;
    passDesc.prog = prog;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    passDesc.pipelineSpecData = (void*)pConst->data;

    std::string passName = getPassName("triangle_culling", _stage, RenderPipeline::mixed);
    kage::PassHandle pass = kage::registPass(passName.c_str(), passDesc);

    kage::BufferDesc triPayloadBuf;
    triPayloadBuf.size = 512 * 1024 * 1024; // 512M
    triPayloadBuf.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    triPayloadBuf.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle triPayload = kage::registBuffer("tri_payload", triPayloadBuf);

    // 2 dispatch indirect commands, 1st: for hardware rasterization, 2nd: for software rasterization
    kage::BufferDesc  trianglePayloadCntDesc;
    trianglePayloadCntDesc.size = sizeof(IndirectDispatchCommand) * 2;
    trianglePayloadCntDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
    trianglePayloadCntDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle trianglePayloadCntBuf = kage::registBuffer("triangle_payload_cnt", trianglePayloadCntDesc);

    kage::BufferHandle triPayloadBufOutAlias = kage::alias(triPayload);
    kage::BufferHandle trianglePayloadCntOutAlias = kage::alias(trianglePayloadCntBuf);

    kage::bindBuffer(pass
        , _initData.meshletPayloadBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::setIndirectBuffer(pass, _initData.meshletPayloadCntBuf);

    kage::bindBuffer(pass
        , _initData.meshDrawBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        , _initData.transformBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        , _initData.vtxBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        , _initData.meshletBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        , _initData.meshletDataBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        , _initData.meshletPayloadCntBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    // write buffers
    kage::bindBuffer(pass
        , triPayload
        , Stage::compute_shader
        , Access::shader_write
        , triPayloadBufOutAlias
    );

    kage::bindBuffer(pass
        , trianglePayloadCntBuf
        , Stage::compute_shader
        , Access::shader_write
        , trianglePayloadCntOutAlias
    );

    kage::SamplerHandle pyrSamp = kage::sampleImage( pass
            , _initData.pyramid
            , Stage::compute_shader
            , kage::SamplerFilter::linear
            , kage::SamplerMipmapMode::nearest
            , kage::SamplerAddressMode::clamp_to_edge
            , kage::SamplerReductionMode::min
    );

    _tric.cs = cs;
    _tric.prog = prog;
    _tric.pass = pass;

    _tric.stage = _stage;

    // read-only
    _tric.meshletPayloadBuf = _initData.meshletPayloadBuf;
    _tric.meshletPayloadCntBuf = _initData.meshletPayloadCntBuf;
    _tric.meshDrawBuf = _initData.meshDrawBuf;
    _tric.transformBuf = _initData.transformBuf;
    _tric.vtxBuf = _initData.vtxBuf;
    _tric.meshletBuf = _initData.meshletBuf;
    _tric.meshletDataBuf = _initData.meshletDataBuf;

    _tric.pyramid = _initData.pyramid;
    _tric.pyrSampler = pyrSamp;
    
    // read-write
    _tric.triPayloadBuf = triPayload;
    _tric.triCountBuf = trianglePayloadCntBuf;
    
    // out-alias
    _tric.triBufOutAlias = triPayloadBufOutAlias;
    _tric.triCountBufOutAlias = trianglePayloadCntOutAlias;

}

void recTriangleCulling(const TriangleCulling& _tric, const Constants& _consts)
{
    KG_ZoneScopedC(kage::Color::blue);

    const kage::Memory* mem = kage::alloc(sizeof(Constants));

    bx::memCopy(mem->data, &_consts, mem->size);

    kage::startRec(_tric.pass);
    kage::setConstants(mem);

    // clear payload buffer in late pass
    if (PassStage::late == _tric.stage)
    {
        kage::fillBuffer(_tric.triPayloadBuf, 0);
    }

    kage::Binding binds[] =
    {
        { _tric.meshletPayloadBuf,      BindingAccess::read,        Stage::compute_shader },
        { _tric.meshDrawBuf,            BindingAccess::read,        Stage::compute_shader },
        { _tric.transformBuf,           BindingAccess::read,        Stage::compute_shader },
        { _tric.vtxBuf,                 BindingAccess::read,        Stage::compute_shader },
        { _tric.meshletBuf,             BindingAccess::read,        Stage::compute_shader },
        { _tric.meshletDataBuf,         BindingAccess::read,        Stage::compute_shader },
        { _tric.meshletPayloadCntBuf,   BindingAccess::read,        Stage::compute_shader },
        { _tric.triPayloadBuf,          BindingAccess::write,       Stage::compute_shader },
        { _tric.triCountBuf,            BindingAccess::write,       Stage::compute_shader },
        { _tric.pyramid,                _tric.pyrSampler,           Stage::compute_shader }
    };
    kage::pushBindings(binds, COUNTOF(binds));
    
    kage::dispatchIndirect(_tric.meshletPayloadCntBuf, offsetof(IndirectDispatchCommand, x));
    kage::endRec();
}

void updateTriangleCulling(TriangleCulling& _tric, const Constants& _consts)
{
    recTriangleCulling(_tric, _consts);
}

