#include "vkz_culling_pass.h"

void prepareCullingComp(CullingComp& _cullingComp, const CullingCompInitData& _initData, bool _late /*= false*/)
{
    vkz::ShaderHandle cs = vkz::registShader("mesh_draw_cmd_shader", "shaders/drawcmd.comp.spv");
    vkz::ProgramHandle prog = vkz::registProgram("mesh_draw_cmd_prog", { cs }, sizeof(MeshDrawCullVKZ));

    int pipelineSpecs[] = { _late, true };

    const vkz::Memory* pConst = vkz::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    vkz::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = vkz::PassExeQueue::compute;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    passDesc.pipelineSpecData = (void*)pConst->data;

    const char* passName = _late ? "cull_pass_late" : "cull_pass_early";
    vkz::PassHandle pass = vkz::registPass(passName, passDesc);

    vkz::BufferHandle drawCmdOutAlias = vkz::alias(_initData.meshDrawCmdBuf);
    vkz::BufferHandle drawCmdCountOutAlias = vkz::alias(_initData.meshDrawCmdCountBuf);
    vkz::BufferHandle drawVisOutAlias = vkz::alias(_initData.meshDrawVisBuf);

    vkz::bindBuffer(pass, _initData.meshBuf
        , 0
        , vkz::PipelineStageFlagBits::compute_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.meshDrawBuf
        , 1
        , vkz::PipelineStageFlagBits::compute_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.transBuf
        , 2
        , vkz::PipelineStageFlagBits::compute_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.meshDrawCmdBuf
        , 3
        , vkz::PipelineStageFlagBits::compute_shader
        , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
        , drawCmdOutAlias);

    vkz::bindBuffer(pass, _initData.meshDrawCmdCountBuf
        , 4
        , vkz::PipelineStageFlagBits::compute_shader
        , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
        , drawCmdCountOutAlias);

    vkz::bindBuffer(pass, _initData.meshDrawVisBuf
        , 5
        , vkz::PipelineStageFlagBits::compute_shader
        , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
        , drawVisOutAlias);

    vkz::sampleImage(pass, _initData.pyramid
        , 6
        , vkz::PipelineStageFlagBits::compute_shader
        , vkz::ImageLayout::general
        , vkz::SamplerReductionMode::weighted_average);

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

    const vkz::Memory* mem = vkz::alloc(sizeof(MeshDrawCullVKZ));
    memcpy(mem->data, &_drawCull, mem->size);
    vkz::updatePushConstants(_cullingComp.pass, mem);
}

