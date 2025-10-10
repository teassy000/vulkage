#include "vkz_culling_pass.h"
#include "vkz_pass.h"

void recMeshCulling(const MeshCulling& _cull, uint32_t _drawCount)
{
    KG_ZoneScopedC(kage::Color::blue);

    const kage::Memory* mem = kage::alloc(sizeof(DrawCull));
    bx::memCopy(mem->data, &_cull.drawCull, mem->size);

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

// get pass name based on culling stage and pass type
void getPassName(std::string& _out, const char* _baseName, RenderStage _stage, RenderPipeline _pass)
{
    _out = _baseName;

    const char* stageStr[static_cast<uint8_t>(RenderStage::count)] = { "_early", "_late", "_alpha" };
    const char* passStr[static_cast<uint8_t>(RenderPipeline::count)] = { "_normal", "_task", "_compute" };
   
    
    _out += stageStr[static_cast<uint8_t>(_stage)];
    _out += passStr[static_cast<uint8_t>(_pass)];
}

void initMeshCulling(MeshCulling& _cullingComp, const MeshCullingInitData& _initData, RenderStage _stage, RenderPipeline _pass)
{
    kage::ShaderHandle cs = kage::registShader("mesh_draw_cmd", "shader/drawcmd.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("mesh_draw_cmd", { cs }, sizeof(DrawCull));

    int pipelineSpecs[] = { 
        _stage == RenderStage::late // LATE
        , _pass == RenderPipeline::task // TASK
        , _stage == RenderStage::alpha // ALPHA_PASS
        , _pass == RenderPipeline::compute // USE_MIXED_RASTER
    };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc;
    passDesc.prog = prog;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    passDesc.pipelineSpecData = (void*)pConst->data;

    std::string passName;
    getPassName(passName, "mesh_culling", _stage, _pass);
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

    _cullingComp.drawCull = {};
}

void updateMeshCulling(MeshCulling& _cullingComp, const DrawCull& _drawCull, uint32_t _drawCount)
{
    _cullingComp.drawCull = _drawCull;

    recMeshCulling(_cullingComp, _drawCount);
}

void initMeshletCulling(MeshletCulling& _cullingComp, const MeshletCullingInitData& _initData, RenderStage _stage, bool _seamless /*= false*/)
{
    kage::ShaderHandle cs = kage::registShader("meshlet_culling", "shader/culling_meshlet.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("meshlet_culling", { cs }, sizeof(DrawCull));

    int pipelineSpecs[] = {
        _stage == RenderStage::late
        , _stage == RenderStage::alpha
        , _seamless
    };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc;
    passDesc.prog = prog;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    passDesc.pipelineSpecData = (void*)pConst->data;

    std::string passName;
    getPassName(passName, "meshlet_culling", _stage, RenderPipeline::compute);
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

void recMeshletCulling(const MeshletCulling& _mltc, const DrawCull& _drawCull)
{
    KG_ZoneScopedC(kage::Color::blue);

    const kage::Memory* mem = kage::alloc(sizeof(DrawCull));
    bx::memCopy(mem->data, &_drawCull, mem->size);

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

void updateMeshletCulling(MeshletCulling& _mltc, const DrawCull& _drawCull)
{
    recMeshletCulling(_mltc, _drawCull);
}

void initTriangleCulling(TriangleCulling& _tric, const TriangleCullingInitData& _initData, RenderStage _stage, bool _seamless /*= false*/)
{
    kage::ShaderHandle cs = kage::registShader("triangle_culling", "shader/culling_triangle.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("triangle_culling", { cs }, sizeof(vec2));

    int pipelineSpecs[] = {
        _stage == RenderStage::alpha
        , _seamless
    };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc;
    passDesc.prog = prog;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    passDesc.pipelineSpecData = (void*)pConst->data;

    std::string passName;
    getPassName(passName, "triangle_culling", _stage, RenderPipeline::compute);
    kage::PassHandle pass = kage::registPass(passName.c_str(), passDesc);

    kage::BufferDesc trianglePayloadBufDesc;
    trianglePayloadBufDesc.size = 128 * 1024 * 1024; // 128M
    trianglePayloadBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    trianglePayloadBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle trianglePayload = kage::registBuffer("triangle_payload", trianglePayloadBufDesc);

    // 2 dispatch indirect commands, 1st: for vertex count, 2nd: for triangle count
    kage::BufferDesc  trianglePayloadCntDesc;
    trianglePayloadCntDesc.size = 32; // the x field is the draw count, then cmd resolution
    trianglePayloadCntDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
    trianglePayloadCntDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle trianglePayloadCntBuf = kage::registBuffer("triangle_payload_cnt", trianglePayloadCntDesc);

    kage::BufferHandle trianglePayloadBufOutAlias = kage::alias(trianglePayload);
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
        , trianglePayload
        , Stage::compute_shader
        , Access::shader_write
        , trianglePayloadBufOutAlias
    );

    kage::bindBuffer(pass
        , trianglePayloadCntBuf
        , Stage::compute_shader
        , Access::shader_write
        , trianglePayloadCntOutAlias
    );

    _tric.cs = cs;
    _tric.prog = prog;
    _tric.pass = pass;

    // read-only
    _tric.meshletPayloadBuf = _initData.meshletPayloadBuf;
    _tric.meshletPayloadCntBuf = _initData.meshletPayloadCntBuf;
    _tric.meshDrawBuf = _initData.meshDrawBuf;
    _tric.transformBuf = _initData.transformBuf;
    _tric.vtxBuf = _initData.vtxBuf;
    _tric.meshletBuf = _initData.meshletBuf;
    _tric.meshletDataBuf = _initData.meshletDataBuf;

    
    // read-write
    _tric.trianglePayloadBuf = trianglePayload;
    _tric.payloadCntBuf = trianglePayloadCntBuf;
    
    // out-alias
    _tric.cmdBufOutAlias = trianglePayloadBufOutAlias;
    _tric.CmdCountBufOutAlias = trianglePayloadCntOutAlias;

}

void recTriangleCulling(const TriangleCulling& _tric)
{
    KG_ZoneScopedC(kage::Color::blue);

    const kage::Memory* mem = kage::alloc(sizeof(vec2));
    vec2 screenSize = { _tric.screenWidth, _tric.screenHeight };
    bx::memCopy(mem->data, &screenSize, mem->size);
    
    kage::startRec(_tric.pass);
    kage::setConstants(mem);

    kage::fillBuffer(_tric.payloadCntBuf, 0);

    kage::Binding binds[] =
    {
        { _tric.meshletPayloadBuf,      BindingAccess::read,       Stage::compute_shader },
        { _tric.meshDrawBuf,            BindingAccess::read,       Stage::compute_shader },
        { _tric.transformBuf,           BindingAccess::read,       Stage::compute_shader },
        { _tric.vtxBuf,                 BindingAccess::read,       Stage::compute_shader },
        { _tric.meshletBuf,             BindingAccess::read,       Stage::compute_shader },
        { _tric.meshletDataBuf,         BindingAccess::read,       Stage::compute_shader },
        { _tric.meshletPayloadCntBuf,   BindingAccess::read_write, Stage::compute_shader },
        { _tric.trianglePayloadBuf,     BindingAccess::read_write, Stage::compute_shader },
        { _tric.payloadCntBuf,          BindingAccess::read_write, Stage::compute_shader },
    };
    kage::pushBindings(binds, COUNTOF(binds));
    
    kage::dispatchIndirect(_tric.meshletPayloadCntBuf, offsetof(IndirectDispatchCommand, x));
    kage::endRec();
}

void updateTriangleCulling(TriangleCulling& _tric, float _width, float _height)
{
    _tric.screenWidth = _width;
    _tric.screenHeight = _height;
    recTriangleCulling(_tric);
}

