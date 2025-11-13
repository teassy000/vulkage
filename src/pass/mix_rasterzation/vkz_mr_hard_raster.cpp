#include "vkz_mr_hard_raster.h"
#include "vkz_pass.h"


void initHardRaster(HardRaster& _hr, const HardRasterInitData& _init, const PassStage _stage)
{
    bool isLate = (PassStage::late == _stage);
    bool isAlpha = (PassStage::alpha == _stage);

    kage::ShaderHandle ms = kage::registShader("hard_raster_mesh", "shader/hard_raster.mesh.spv");
    kage::ShaderHandle fs = kage::registShader("hard_raster_frag", "shader/bindless.frag.spv");

    kage::ProgramHandle prog = kage::registProgram("hard_raster_prog", { ms, fs }, sizeof(Constants), _init.bindless);

    kage::PassDesc desc{};
    desc.prog = prog;
    desc.queue = kage::PassExeQueue::graphics;
    desc.pipelineConfig.depthCompOp = kage::CompareOp::greater;
    desc.pipelineConfig.enableDepthTest = true;
    desc.pipelineConfig.enableDepthWrite = true;
    desc.pipelineConfig.cullMode = isAlpha ? kage::CullModeFlagBits::none : kage::CullModeFlagBits::back;

    std::string passNameStr = getPassName("hard_raster", _stage);
    kage::PassHandle pass = kage::registPass(passNameStr.c_str(), desc);

    kage::ImageHandle depthOutAlias = kage::alias(_init.depth);
    GBuffer gBufferOutAlias = aliasGBuffer(_init.g_buffer);

    kage::bindBuffer(pass, _init.meshDrawBuffer
        , Stage::mesh_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _init.transformBuffer
        , Stage::mesh_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _init.vtxBuffer
        , Stage::mesh_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _init.meshletBuffer
        , Stage::mesh_shader
        , Access::shader_read);


    kage::bindBuffer(pass, _init.meshletDataBuffer
        , Stage::mesh_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _init.triPayloadBuffer
        , Stage::mesh_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass, _init.triPayloadCountBuffer
        , Stage::mesh_shader
        , Access::shader_read
    );

    kage::SamplerHandle pyrSampler = kage::sampleImage(pass, _init.pyramid
        , Stage::fragment_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );


    kage::setIndirectBuffer(pass, _init.triPayloadCountBuffer);


    kage::setAttachmentOutput(pass, _init.depth, depthOutAlias);

    // bind g-buffer
    kage::setAttachmentOutput(pass, _init.g_buffer.albedo, gBufferOutAlias.albedo);
    kage::setAttachmentOutput(pass, _init.g_buffer.normal, gBufferOutAlias.normal);
    kage::setAttachmentOutput(pass, _init.g_buffer.worldPos, gBufferOutAlias.worldPos);
    kage::setAttachmentOutput(pass, _init.g_buffer.emissive, gBufferOutAlias.emissive);
    kage::setAttachmentOutput(pass, _init.g_buffer.specular, gBufferOutAlias.specular);

    _hr.pass = pass;
    _hr.ms = ms;
    _hr.fs = fs;
    _hr.program = prog;

    // configure
    _hr.pipeline = RenderPipeline::mixed;
    _hr.stage = _stage;

    // read-only buffers
    _hr.vtxBuffer = _init.vtxBuffer;
    _hr.meshBuffer = _init.meshBuffer;
    _hr.meshletBuffer = _init.meshletBuffer;
    _hr.meshletDataBuffer = _init.meshletDataBuffer;
    _hr.meshDrawBuffer = _init.meshDrawBuffer;
    _hr.transformBuffer = _init.transformBuffer;
    _hr.triPayloadBuffer = _init.triPayloadBuffer;
    _hr.triPayloadCountBuffer = _init.triPayloadCountBuffer;

    _hr.pyramid = _init.pyramid;
    _hr.pyrSampler = pyrSampler;
    _hr.bindless = _init.bindless;

    // read / write images
    _hr.depth = _init.depth;
    _hr.g_buffer = _init.g_buffer;

    // out-alias
    _hr.depthOutAlias = depthOutAlias;
    _hr.g_bufferOutAlias = gBufferOutAlias;
}

void recHardRaster(const HardRaster& _hr, const Constants& _consts)
{
    KG_ZoneScopedC(kage::Color::blue);

    kage::startRec(_hr.pass);

    const kage::Memory* mem = kage::alloc(sizeof(Constants));
    memcpy(mem->data, &_consts, mem->size);
    kage::setConstants(mem);

    kage::Binding binds[] =
    {
        { _hr.transformBuffer,      BindingAccess::read,    Stage::mesh_shader },
        { _hr.vtxBuffer,            BindingAccess::read,    Stage::mesh_shader },
        { _hr.meshDrawBuffer,       BindingAccess::read,    Stage::mesh_shader | Stage::fragment_shader },
        { _hr.meshletBuffer,        BindingAccess::read,    Stage::mesh_shader },
        { _hr.meshletDataBuffer,    BindingAccess::read,    Stage::mesh_shader },
        { _hr.triPayloadBuffer,     BindingAccess::read,    Stage::mesh_shader },
        { _hr.triPayloadCountBuffer,BindingAccess::read,    Stage::mesh_shader },
    };

    kage::pushBindings(binds, COUNTOF(binds));

    kage::setBindless(_hr.bindless);

    kage::setViewport(0, 0, (uint32_t)_consts.screenWidth, (uint32_t)_consts.screenHeight);
    kage::setScissor(0, 0, (uint32_t)_consts.screenWidth, (uint32_t)_consts.screenHeight);

    bool is_early = (PassStage::early == _hr.stage);

    kage::Attachment attachments[] = {
        {_hr.g_buffer.albedo,   is_early ? LoadOp::clear : LoadOp::dont_care, StoreOp::store},
        {_hr.g_buffer.normal,   is_early ? LoadOp::clear : LoadOp::dont_care, StoreOp::store},
        {_hr.g_buffer.worldPos, is_early ? LoadOp::clear : LoadOp::dont_care, StoreOp::store},
        {_hr.g_buffer.emissive, is_early ? LoadOp::clear : LoadOp::dont_care, StoreOp::store},
        {_hr.g_buffer.specular, is_early ? LoadOp::clear : LoadOp::dont_care, StoreOp::store},
    };
    kage::setColorAttachments(attachments, COUNTOF(attachments));

    kage::Attachment depthAttachment = {
        _hr.depth
        , is_early ? LoadOp::clear : LoadOp::dont_care
        , StoreOp::store
    };
    kage::setDepthAttachment(depthAttachment);

    kage::drawMeshTask(_hr.triPayloadCountBuffer, offsetof(IndirectDispatchCommand, x), 1, sizeof(IndirectDispatchCommand));

    kage::endRec();

}

void updateHardRaster(HardRaster& _hr, const Constants& _consts)
{
    recHardRaster(_hr, _consts);
}