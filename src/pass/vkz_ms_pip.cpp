
#include "vkz_ms_pip.h"
#include "vkz_pass.h"

#include "demo_structs.h"
#include "scene/scene.h"
#include "core/kage_math.h"

#include "bx/readerwriter.h"

void meshShadingRec(const MeshShading& _ms)
{
    KG_ZoneScopedC(kage::Color::blue);

    const kage::Memory* mem = kage::alloc(sizeof(Constants));
    memcpy(mem->data, &_ms.constants, mem->size);

    kage::Binding binds[] =
    {
        {_ms.meshDrawCmdBuffer, BindingAccess::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.meshBuffer,        BindingAccess::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.meshDrawBuffer,    BindingAccess::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.transformBuffer,   BindingAccess::read,       Stage::task_shader | Stage::mesh_shader | Stage::fragment_shader},
        {_ms.vtxBuffer,         BindingAccess::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.meshletBuffer,     BindingAccess::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.meshletDataBuffer, BindingAccess::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.meshletVisBuffer,  BindingAccess::read_write, Stage::task_shader | Stage::mesh_shader},
        {_ms.pyramid,       _ms.pyramidSampler,     Stage::task_shader | Stage::mesh_shader}
    };

    kage::startRec(_ms.pass);

    kage::setConstants(mem);
    kage::pushBindings(binds, COUNTOF(binds));

    kage::setBindless(_ms.bindless);

    kage::setViewport(0, 0, (uint32_t)_ms.constants.screenWidth, (uint32_t)_ms.constants.screenHeight);
    kage::setScissor(0, 0, (uint32_t)_ms.constants.screenWidth, (uint32_t)_ms.constants.screenHeight);

    bool is_early = (PassStage::early == _ms.stage);

    kage::Attachment attachments[] = {
        {_ms.g_buffer.albedo,   is_early ? LoadOp::clear : LoadOp::dont_care, StoreOp::store},
        {_ms.g_buffer.normal,   is_early ? LoadOp::clear : LoadOp::dont_care, StoreOp::store},
        {_ms.g_buffer.worldPos, is_early ? LoadOp::clear : LoadOp::dont_care, StoreOp::store},
        {_ms.g_buffer.emissive, is_early ? LoadOp::clear : LoadOp::dont_care, StoreOp::store},
        {_ms.g_buffer.specular, is_early ? LoadOp::clear : LoadOp::dont_care, StoreOp::store},
    };
    kage::setColorAttachments(attachments, COUNTOF(attachments));

    kage::Attachment depthAttachment = { 
        _ms.depth
        , is_early ? LoadOp::clear : LoadOp::dont_care
        , StoreOp::store 
    };
    kage::setDepthAttachment(depthAttachment);

    kage::drawMeshTask(_ms.meshDrawCmdCountBuffer, 4, 1, 0);

    kage::endRec();
}

void prepareMeshShading(MeshShading& _meshShading, const Scene& _scene, uint32_t _width, uint32_t _height, const MeshShadingInitData _initData, const PassStage _stage)
{
    bool isLate = (PassStage::late == _stage);
    bool isAlpha = (PassStage::alpha == _stage);

    kage::ShaderHandle ms= kage::registShader("mesh_shader", "shader/meshlet.mesh.spv");
    kage::ShaderHandle ts = kage::registShader("task_shader", "shader/meshlet.task.spv");
    kage::ShaderHandle fs = kage::registShader("mesh_frag_shader", "shader/bindless.frag.spv");

    kage::ProgramHandle prog = kage::registProgram("mesh_prog", { ts, ms, fs }, sizeof(Constants), _initData.bindless);

    int pipelineSpecs[] = { 
        isLate
        , isAlpha
        , {kage::kSeamlessLod == 1} };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc desc{};
    desc.prog = prog;
    desc.queue = kage::PassExeQueue::graphics;
    desc.pipelineConfig.depthCompOp = kage::CompareOp::greater;
    desc.pipelineConfig.enableDepthTest = true;
    desc.pipelineConfig.enableDepthWrite = true;
    desc.pipelineConfig.cullMode = isAlpha ? kage::CullModeFlagBits::none : kage::CullModeFlagBits::back;

    desc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    desc.pipelineSpecData = (void*)pConst->data;

    std::string passNameStr = getPassName("mesh_shading", _stage);
    kage::PassHandle pass = kage::registPass(passNameStr.c_str(), desc);

    kage::BufferHandle mltVisBufOutAlias = kage::alias(_initData.meshletVisBuffer);
    kage::ImageHandle depthOutAlias = kage::alias(_initData.depth);
    GBuffer gb_outAlias = aliasGBuffer(_initData.g_buffer);

    kage::bindBuffer(pass, _initData.meshDrawCmdBuffer
        , Stage::task_shader
        , Access::shader_read);
    
    kage::bindBuffer(pass, _initData.meshBuffer
        , Stage::task_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _initData.meshDrawBuffer
        , Stage::task_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _initData.transformBuffer
        , Stage::task_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _initData.vtxBuffer
        , Stage::task_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _initData.meshletBuffer
        , Stage::task_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _initData.meshletDataBuffer
        , Stage::task_shader
        , Access::shader_read);

    kage::bindBuffer(pass, _initData.meshletVisBuffer
        , Stage::task_shader
        , Access::shader_read | Access::shader_write
        , mltVisBufOutAlias);

    kage::SamplerHandle pyrSampler = kage::sampleImage(pass, _initData.pyramid
        , Stage::fragment_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::setIndirectBuffer(pass, _initData.meshDrawCmdCountBuffer);


    kage::setAttachmentOutput(pass, _initData.depth, depthOutAlias);

    // bind g-buffer
    kage::setAttachmentOutput(pass, _initData.g_buffer.albedo, gb_outAlias.albedo);
    kage::setAttachmentOutput(pass, _initData.g_buffer.normal, gb_outAlias.normal);
    kage::setAttachmentOutput(pass, _initData.g_buffer.worldPos, gb_outAlias.worldPos);
    kage::setAttachmentOutput(pass, _initData.g_buffer.emissive, gb_outAlias.emissive);
    kage::setAttachmentOutput(pass, _initData.g_buffer.specular, gb_outAlias.specular);

    // set the data
    _meshShading.stage = _stage;

    _meshShading.taskShader = ts;
    _meshShading.meshShader = ms;
    _meshShading.fragShader = fs;
    _meshShading.program = prog;
    _meshShading.pass = pass;

    _meshShading.vtxBuffer = _initData.vtxBuffer;
    _meshShading.meshBuffer = _initData.meshBuffer;
    _meshShading.meshletBuffer = _initData.meshletBuffer;
    _meshShading.meshletDataBuffer = _initData.meshletDataBuffer;
    _meshShading.meshDrawCmdBuffer = _initData.meshDrawCmdBuffer;
    _meshShading.meshDrawCmdCountBuffer = _initData.meshDrawCmdCountBuffer;
    _meshShading.meshDrawBuffer = _initData.meshDrawBuffer;
    _meshShading.transformBuffer = _initData.transformBuffer;

    _meshShading.pyramidSampler = pyrSampler;

    _meshShading.bindless = _initData.bindless;

    _meshShading.meshletVisBuffer = _initData.meshletVisBuffer;
    _meshShading.pyramid = _initData.pyramid;
    _meshShading.depth = _initData.depth;
    _meshShading.g_buffer = _initData.g_buffer;

    _meshShading.meshletVisBufferOutAlias = mltVisBufOutAlias;
    _meshShading.depthOutAlias = depthOutAlias;
    _meshShading.g_bufferOutAlias = gb_outAlias;
}

void updateMeshShading(MeshShading& _meshShading, const Constants& _consts)
{
    _meshShading.constants = _consts;

    meshShadingRec(_meshShading);
}