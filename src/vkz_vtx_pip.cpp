#include "vkz_vtx_pip.h"

using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;

void vtxShadingRec(VtxShading& _v)
{
    KG_ZoneScopedC(kage::Color::blue);

    const kage::Memory* mem = kage::alloc(sizeof(Globals));
    memcpy(mem->data, &_v.globals, mem->size);

    kage::Binding binds[] =
    {
        {_v.meshDrawCmdBuf, Access::read,       Stage::vertex_shader},
        {_v.meshDrawBuf,    Access::read,       Stage::vertex_shader},
        {_v.vtxBuf,         Access::read,       Stage::vertex_shader},
        {_v.transformBuf,   Access::read,       Stage::vertex_shader},
    };

    kage::startRec(_v.pass);

    kage::setConstants(mem);
    kage::pushBindings(binds, COUNTOF(binds));

    kage::setViewport(0, 0, (uint32_t)_v.globals.screenWidth, (uint32_t)_v.globals.screenHeight);
    kage::setScissor(0, 0, (uint32_t)_v.globals.screenWidth, (uint32_t)_v.globals.screenHeight);

    kage::setVertexBuffer(_v.vtxBuf);
    kage::setIndexBuffer(_v.idxBuf, 0, kage::IndexType::uint32);

    kage::drawIndexed(
        _v.meshDrawCmdBuf
        , offsetof(MeshDrawCommand, indexCount)
        , _v.meshDrawCmdCountBuf
        , 0
        , _v.maxMeshDrawCmdCount
        , sizeof(MeshDrawCommand)
    );

    kage::endRec();
}

void prepareVtxShading(VtxShading& _vtxShading, const Scene& _scene, const VtxShadingInitData& _initData, bool _late /*= false*/)
{
    // render shader
    kage::ShaderHandle vs = kage::registShader("mesh_vert_shader", "shaders/mesh.vert.spv");
    kage::ShaderHandle fs = kage::registShader("mesh_frag_shader", "shaders/mesh.frag.spv");
    kage::ProgramHandle prog = kage::registProgram("mesh_prog", { vs, fs }, sizeof(Globals));
    // pass
    kage::PassDesc desc;
    desc.programId = prog.id;
    desc.queue = kage::PassExeQueue::graphics;
    desc.pipelineConfig.depthCompOp = kage::CompareOp::greater;
    desc.pipelineConfig.enableDepthTest = true;
    desc.pipelineConfig.enableDepthWrite = true;

    desc.passConfig.colorLoadOp = _late ? kage::AttachmentLoadOp::dont_care : kage::AttachmentLoadOp::clear;
    desc.passConfig.colorStoreOp = kage::AttachmentStoreOp::store;
    desc.passConfig.depthLoadOp = _late ? kage::AttachmentLoadOp::dont_care : kage::AttachmentLoadOp::clear;
    desc.passConfig.depthStoreOp = kage::AttachmentStoreOp::store;

    kage::PassHandle pass = kage::registPass("vtx_render_pass", desc);

    kage::BufferHandle meshDrawCmdBufOutAlias = kage::alias(_initData.meshDrawCmdBuf);
    kage::ImageHandle colorOutAlias = kage::alias(_initData.color);
    kage::ImageHandle depthOutAlias = kage::alias(_initData.depth);


    // index buffer
    kage::bindIndexBuffer(pass, _initData.idxBuf);

    // bindings
    kage::bindBuffer(pass, _initData.meshDrawCmdBuf
        , 0
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.meshDrawBuf
        , 1
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.vtxBuf
        , 2
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.transformBuf
        , 3
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read);


    kage::setIndirectBuffer(pass, _initData.meshDrawCmdBuf, offsetof(MeshDrawCommand, indexCount), sizeof(MeshDrawCommand), (uint32_t)_scene.meshDraws.size());
    kage::setIndirectCountBuffer(pass, _initData.meshDrawCmdCountBuf, 0);

    kage::setAttachmentOutput(pass, _initData.color, 0, colorOutAlias);
    kage::setAttachmentOutput(pass, _initData.depth, 0, depthOutAlias);

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

    _vtxShading.maxMeshDrawCmdCount = (uint32_t)_scene.meshDraws.size();
}

void updateVtxShadingConstants(VtxShading& _vtxShading, const Globals& _globals)
{
    _vtxShading.globals = _globals;

    vtxShadingRec(_vtxShading);
}
