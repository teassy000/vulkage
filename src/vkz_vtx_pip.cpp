#include "vkz_vtx_pip.h"


void prepareVtxShading(VtxShading& _vtxShading, const Scene& _scene, const VtxShadingInitData& _initData, bool _late /*= false*/)
{
    // render shader
    vkz::ShaderHandle vs = vkz::registShader("mesh_vert_shader", "shaders/mesh.vert.spv");
    vkz::ShaderHandle fs = vkz::registShader("mesh_frag_shader", "shaders/mesh.frag.spv");
    vkz::ProgramHandle prog = vkz::registProgram("mesh_prog", { vs, fs }, sizeof(GlobalsVKZ));
    // pass
    vkz::PassDesc desc;
    desc.programId = prog.id;
    desc.queue = vkz::PassExeQueue::graphics;
    desc.pipelineConfig.depthCompOp = vkz::CompareOp::greater;
    desc.pipelineConfig.enableDepthTest = true;
    desc.pipelineConfig.enableDepthWrite = true;

    desc.passConfig.colorLoadOp = _late ? vkz::AttachmentLoadOp::load : vkz::AttachmentLoadOp::clear;
    desc.passConfig.colorStoreOp = vkz::AttachmentStoreOp::store;
    desc.passConfig.depthLoadOp = _late ? vkz::AttachmentLoadOp::load : vkz::AttachmentLoadOp::clear;
    desc.passConfig.depthStoreOp = vkz::AttachmentStoreOp::store;

    vkz::PassHandle pass = vkz::registPass("vtx_render_pass", desc);

    vkz::BufferHandle meshDrawCmdBufOutAlias = vkz::alias(_initData.meshDrawCmdBuf);
    vkz::ImageHandle colorOutAlias = vkz::alias(_initData.color);
    vkz::ImageHandle depthOutAlias = vkz::alias(_initData.depth);


    // index buffer
    vkz::bindIndexBuffer(pass, _initData.idxBuf);

    // bindings
    vkz::bindBuffer(pass, _initData.meshDrawCmdBuf
        , 0
        , vkz::PipelineStageFlagBits::vertex_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.meshDrawBuf
        , 1
        , vkz::PipelineStageFlagBits::vertex_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.vtxBuf
        , 2
        , vkz::PipelineStageFlagBits::vertex_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.transformBuf
        , 3
        , vkz::PipelineStageFlagBits::vertex_shader
        , vkz::AccessFlagBits::shader_read);


    vkz::setIndirectBuffer(pass, _initData.meshDrawCmdBuf, offsetof(MeshDrawCommandVKZ, indexCount), sizeof(MeshDrawCommandVKZ), (uint32_t)_scene.meshDraws.size());
    vkz::setIndirectCountBuffer(pass, _initData.meshDrawCmdCountBuf, 0);

    vkz::setAttachmentOutput(pass, _initData.color, 0, colorOutAlias);
    vkz::setAttachmentOutput(pass, _initData.depth, 0, depthOutAlias);

    _vtxShading.vtxShader = vs;
    _vtxShading.fragShader = fs;
    _vtxShading.prog = prog;
    _vtxShading.pass = pass;

    _vtxShading.idxBuf = _initData.idxBuf;
    _vtxShading.vtxBuf = _initData.vtxBuf;
    _vtxShading.meshDrawBuf = _initData.meshDrawBuf;
    _vtxShading.meshDrawCmdBuf = _initData.meshDrawCmdBuf;
    _vtxShading.transformBuf = _initData.transformBuf;
    _vtxShading.meshDrawCmdCountBuf = _initData.meshDrawCmdCountBuf;
    _vtxShading.color = _initData.color;
    _vtxShading.depth = _initData.depth;

    _vtxShading.meshDrawCmdBufOutAlias = meshDrawCmdBufOutAlias;
    _vtxShading.colorOutAlias = colorOutAlias;
    _vtxShading.depthOutAlias = depthOutAlias;
}

void updateVtxShadingConstants(VtxShading& _vtxShading, const GlobalsVKZ& _globals)
{
    _vtxShading.globals = _globals;

    const vkz::Memory* mem = vkz::alloc(sizeof(GlobalsVKZ));
    memcpy(mem->data, &_globals, mem->size);
    vkz::updatePushConstants(_vtxShading.pass, mem);
}
