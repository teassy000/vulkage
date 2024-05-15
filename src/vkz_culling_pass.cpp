#include "vkz_culling_pass.h"

void prepareCullingComp(CullingComp& _cullingComp, const CullingCompInitData& _initData, bool _late /*= false*/, bool _task /*= false*/)
{
    kage::ShaderHandle cs = kage::registShader("mesh_draw_cmd_shader", "shaders/drawcmd.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("mesh_draw_cmd_prog", { cs }, sizeof(MeshDrawCullVKZ));

    int pipelineSpecs[] = { _late, _task };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    passDesc.pipelineSpecData = (void*)pConst->data;

    const char* passName = _late ? "cull_pass_late" : "cull_pass_early";
    kage::PassHandle pass = kage::registPass(passName, passDesc);

    kage::BufferHandle drawCmdOutAlias = kage::alias(_initData.meshDrawCmdBuf);
    kage::BufferHandle drawCmdCountOutAlias = kage::alias(_initData.meshDrawCmdCountBuf);
    kage::BufferHandle drawVisOutAlias = kage::alias(_initData.meshDrawVisBuf);

    kage::bindBuffer(pass, _initData.meshBuf
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.meshDrawBuf
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.transBuf
        , 2
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.meshDrawCmdBuf
        , 3
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , drawCmdOutAlias);

    kage::bindBuffer(pass, _initData.meshDrawCmdCountBuf
        , 4
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , drawCmdCountOutAlias);

    kage::bindBuffer(pass, _initData.meshDrawVisBuf
        , 5
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , drawVisOutAlias);

    kage::sampleImage(pass, _initData.pyramid
        , 6
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerReductionMode::min);

    _cullingComp.cs = cs;
    _cullingComp.prog = prog;
    _cullingComp.pass = pass;

    _cullingComp.meshBuf = _initData.meshBuf;
    _cullingComp.meshDrawBuf = _initData.meshDrawBuf;
    _cullingComp.transBuf = _initData.transBuf;
    _cullingComp.meshDrawCmdBuf = _initData.meshDrawCmdBuf;
    _cullingComp.meshDrawCmdCountBuf = _initData.meshDrawCmdCountBuf;
    _cullingComp.meshDrawVisBuf = _initData.meshDrawVisBuf;

    _cullingComp.meshDrawCmdBufOutAlias = drawCmdOutAlias;
    _cullingComp.meshDrawCmdCountBufOutAlias = drawCmdCountOutAlias;
    _cullingComp.meshDrawVisBufOutAlias = drawVisOutAlias;

    _cullingComp.meshDrawCull = {};
}

void updateCullingConstants(CullingComp& _cullingComp, const MeshDrawCullVKZ& _drawCull)
{
    _cullingComp.meshDrawCull = _drawCull;

    const kage::Memory* mem = kage::alloc(sizeof(MeshDrawCullVKZ));
    memcpy(mem->data, &_drawCull, mem->size);
    kage::updatePushConstants(_cullingComp.pass, mem);
}

