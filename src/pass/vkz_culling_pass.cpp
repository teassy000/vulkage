#include "vkz_culling_pass.h"

void cullingRec(const MeshCulling& _cull, uint32_t _drawCount)
{
    KG_ZoneScopedC(kage::Color::blue);

    using Stage = kage::PipelineStageFlagBits::Enum;
    using Access = kage::BindingAccess;
    const kage::Memory* mem = kage::alloc(sizeof(DrawCull));
    bx::memCopy(mem->data, &_cull.drawCull, mem->size);

    kage::startRec(_cull.pass);

    kage::fillBuffer(_cull.meshDrawCmdCountBuf, 0);

    kage::setConstants(mem);

    kage::Binding binds[] =
    {
        { _cull.meshBuf,             Access::read,          Stage::compute_shader },
        { _cull.meshDrawBuf,         Access::read,          Stage::compute_shader },
        { _cull.transBuf,            Access::read,          Stage::compute_shader },
        { _cull.meshDrawCmdBuf,      Access::write,         Stage::compute_shader },
        { _cull.meshDrawCmdCountBuf, Access::read_write,    Stage::compute_shader },
        { _cull.meshDrawVisBuf,      Access::read_write,    Stage::compute_shader },
        { _cull.pyramid,             _cull.pyrSampler,      Stage::compute_shader },
    };
    kage::pushBindings(binds, COUNTOF(binds));
    kage::dispatch(_drawCount, 1, 1);

    kage::endRec();
}

// get pass name based on culling stage and pass type
void getPassName(std::string& _out, const char* _baseName, CullingStage _stage, CullingPass _pass)
{
    _out = _baseName;

    const char* stageStr[static_cast<uint8_t>(CullingStage::count)] = { "_early", "_late", "_alpha" };
    const char* passStr[static_cast<uint8_t>(CullingPass::count)] = { "_normal", "_task", "_compute" };
   
    
    _out += stageStr[static_cast<uint8_t>(_stage)];
    _out += passStr[static_cast<uint8_t>(_pass)];
}

void prepareMeshCulling(MeshCulling& _cullingComp, const MeshCullingInitData& _initData, CullingStage _stage, CullingPass _pass)
{
    kage::ShaderHandle cs = kage::registShader("mesh_draw_cmd", "shader/drawcmd.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("mesh_draw_cmd", { cs }, sizeof(DrawCull));

    int pipelineSpecs[] = { 
        _stage == CullingStage::early
        , _pass == CullingPass::task
        , _stage == CullingStage::alpha
        , _pass == CullingPass::compute
    };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    passDesc.pipelineSpecData = (void*)pConst->data;

    std::string passName;
    getPassName(passName, "culling", _stage, _pass);
    kage::PassHandle pass = kage::registPass(passName.c_str(), passDesc);

    kage::BufferHandle drawCmdOutAlias = kage::alias(_initData.meshDrawCmdBuf);
    kage::BufferHandle drawCmdCountOutAlias = kage::alias(_initData.meshDrawCmdCountBuf);
    kage::BufferHandle drawVisOutAlias = kage::alias(_initData.meshDrawVisBuf);

    kage::bindBuffer(pass, _initData.meshBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.meshDrawBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.transBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.meshDrawCmdBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , drawCmdOutAlias);

    kage::bindBuffer(pass, _initData.meshDrawCmdCountBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , drawCmdCountOutAlias);

    kage::bindBuffer(pass, _initData.meshDrawVisBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , drawVisOutAlias);

     kage::SamplerHandle samp = kage::sampleImage(pass, _initData.pyramid
        , kage::PipelineStageFlagBits::compute_shader
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

    _cullingComp.meshDrawCmdBufOutAlias = drawCmdOutAlias;
    _cullingComp.meshDrawCmdCountBufOutAlias = drawCmdCountOutAlias;
    _cullingComp.meshDrawVisBufOutAlias = drawVisOutAlias;

    _cullingComp.drawCull = {};
}

void updateMeshCulling(MeshCulling& _cullingComp, const DrawCull& _drawCull, uint32_t _drawCount)
{
    _cullingComp.drawCull = _drawCull;

    cullingRec(_cullingComp, _drawCount);
}

