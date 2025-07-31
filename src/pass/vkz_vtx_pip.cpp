#include "vkz_vtx_pip.h"

using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;
using LoadOp = kage::AttachmentLoadOp;
using StoreOp = kage::AttachmentStoreOp;

void vtxShadingRec(VtxShading& _v)
{
    KG_ZoneScopedC(kage::Color::blue);

    const kage::Memory* mem = kage::alloc(sizeof(Globals));
    memcpy(mem->data, &_v.globals, mem->size);

    kage::Binding binds[] =
    {
        {_v.meshDrawCmdBuf, Access::read,       Stage::vertex_shader},
        {_v.vtxBuf,         Access::read,       Stage::vertex_shader},
        {_v.meshDrawBuf,    Access::read,       Stage::vertex_shader},
        {_v.transformBuf,   Access::read,       Stage::vertex_shader | Stage::fragment_shader},
    };

    kage::startRec(_v.pass);

    kage::setConstants(mem);
    kage::pushBindings(binds, COUNTOF(binds));
    kage::setBindless(_v.bindless);

    kage::setViewport(0, 0, (uint32_t)_v.globals.screenWidth, (uint32_t)_v.globals.screenHeight);
    kage::setScissor(0, 0, (uint32_t)_v.globals.screenWidth, (uint32_t)_v.globals.screenHeight);

    kage::Attachment attachments[] = {
        {_v.color, _v.late ? LoadOp::dont_care : LoadOp::clear, StoreOp::store},
    };
    kage::setColorAttachments(attachments, COUNTOF(attachments));

    kage::Attachment depthAttachment = {
        _v.depth
        , _v.late ? LoadOp::dont_care : LoadOp::clear
        , StoreOp::store
    };
    kage::setDepthAttachment(depthAttachment);

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
    kage::ShaderHandle fs = kage::registShader("mesh_frag_shader", "shaders/bindless.frag.spv");
    kage::ProgramHandle prog = kage::registProgram("mesh_prog", { vs, fs }, sizeof(Globals), _initData.bindless);
    // pass
    kage::PassDesc desc;
    desc.programId = prog.id;
    desc.queue = kage::PassExeQueue::graphics;
    desc.pipelineConfig.depthCompOp = kage::CompareOp::greater;
    desc.pipelineConfig.enableDepthTest = true;
    desc.pipelineConfig.enableDepthWrite = true;

    kage::PassHandle pass = kage::registPass("vtx_render_pass", desc);

    kage::BufferHandle meshDrawCmdBufOutAlias = kage::alias(_initData.meshDrawCmdBuf);
    kage::ImageHandle colorOutAlias = kage::alias(_initData.color);
    kage::ImageHandle depthOutAlias = kage::alias(_initData.depth);


    // index buffer
    kage::bindIndexBuffer(pass, _initData.idxBuf);

    // bindings
    kage::bindBuffer(pass, _initData.meshDrawCmdBuf
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.vtxBuf
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.meshDrawBuf
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.transformBuf
        , kage::PipelineStageFlagBits::vertex_shader | kage::PipelineStageFlagBits::fragment_shader
        , kage::AccessFlagBits::shader_read);


    kage::setIndirectBuffer(pass, _initData.meshDrawCmdBuf, offsetof(MeshDrawCommand, indexCount), sizeof(MeshDrawCommand), (uint32_t)_scene.meshDraws.size());
    kage::setIndirectCountBuffer(pass, _initData.meshDrawCmdCountBuf, 0);

    kage::setAttachmentOutput(pass, _initData.color, colorOutAlias);
    kage::setAttachmentOutput(pass, _initData.depth, depthOutAlias);


    _vtxShading.late = _late;

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

    _vtxShading.bindless = _initData.bindless;

    _vtxShading.maxMeshDrawCmdCount = (uint32_t)_scene.meshDraws.size();
}

void updateVtxShadingConstants(VtxShading& _vtxShading, const Globals& _globals)
{
    _vtxShading.globals = _globals;

    vtxShadingRec(_vtxShading);
}
