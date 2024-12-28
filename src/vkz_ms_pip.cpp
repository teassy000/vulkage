
#include "vkz_ms_pip.h"
#include "demo_structs.h"
#include "scene.h"
#include "kage_math.h"

#include "bx/readerwriter.h"

using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;
using LoadOp = kage::AttachmentLoadOp;
using StoreOp = kage::AttachmentStoreOp;

void taskSubmitRec(const TaskSubmit& _ts)
{
    KG_ZoneScopedC(kage::Color::blue);

    kage::Binding binds[] =
    {
        {_ts.drawCmdCountBuffer,    Access::read,   Stage::compute_shader},
        {_ts.drawCmdBuffer,         Access::write,  Stage::compute_shader}
    };

    kage::startRec(_ts.pass);

    kage::pushBindings(binds, COUNTOF(binds));

    kage::dispatch(1, 1, 1);

    kage::endRec();
}

void meshShadingRec(const MeshShading& _ms)
{
    KG_ZoneScopedC(kage::Color::blue);

    const kage::Memory* mem = kage::alloc(sizeof(Globals));
    memcpy(mem->data, &_ms.globals, mem->size);

    kage::Binding binds[] =
    {
        {_ms.meshDrawCmdBuffer, Access::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.meshBuffer,        Access::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.meshDrawBuffer,    Access::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.meshletBuffer,     Access::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.meshletDataBuffer, Access::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.vtxBuffer,         Access::read,       Stage::task_shader | Stage::mesh_shader},
        {_ms.transformBuffer,   Access::read,       Stage::task_shader | Stage::mesh_shader | Stage::fragment_shader},
        {_ms.meshletVisBuffer,  Access::read_write, Stage::task_shader | Stage::mesh_shader},
        {_ms.pyramid,       _ms.pyramidSampler,     Stage::task_shader | Stage::mesh_shader}
    };

    kage::startRec(_ms.pass);

    kage::setConstants(mem);
    kage::pushBindings(binds, COUNTOF(binds));

    kage::setBindless(_ms.bindless);

    kage::setViewport(0, 0, (uint32_t)_ms.globals.screenWidth, (uint32_t)_ms.globals.screenHeight);
    kage::setScissor(0, 0, (uint32_t)_ms.globals.screenWidth, (uint32_t)_ms.globals.screenHeight);



    kage::Attachment attachments[] = {
        {_ms.color, LoadOp::dont_care, StoreOp::store},
    };
    kage::setColorAttachments(attachments, COUNTOF(attachments));

    kage::Attachment depthAttachment = { 
        _ms.depth
        , _ms.late ? LoadOp::dont_care : LoadOp::clear
        , StoreOp::store 
    };
    kage::setDepthAttachment(depthAttachment);

    kage::drawMeshTask(_ms.meshDrawCmdCountBuffer, 4, 1, 0);

    kage::endRec();
}

void prepareMeshShading(MeshShading& _meshShading, const Scene& _scene, uint32_t _width, uint32_t _height, const MeshShadingInitData _initData, bool _late /*= false*/, bool _alphaPass /*= false*/)
{
    kage::ShaderHandle ms= kage::registShader("mesh_shader", "shaders/meshlet.mesh.spv");
    kage::ShaderHandle ts = kage::registShader("task_shader", "shaders/meshlet.task.spv");
    kage::ShaderHandle fs = kage::registShader("mesh_frag_shader", "shaders/bindless.frag.spv");

    kage::ProgramHandle prog = kage::registProgram("mesh_prog", { ts, ms, fs }, sizeof(Globals), _initData.bindless);

    int pipelineSpecs[] = { _late, _alphaPass, {kage::kSeamlessLod == 1} };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));


    kage::PassDesc desc{};
    desc.programId = prog.id;
    desc.queue = kage::PassExeQueue::graphics;
    desc.pipelineConfig.depthCompOp = kage::CompareOp::greater;
    desc.pipelineConfig.enableDepthTest = true;
    desc.pipelineConfig.enableDepthWrite = true;

    desc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    desc.pipelineSpecData = (void*)pConst->data;

    const char* passName = 
        _alphaPass 
        ? "mesh_pass_alpha"
        : _late 
            ? "mesh_pass_late" 
            : "mesh_pass_early";
    kage::PassHandle pass = kage::registPass(passName, desc);

    kage::BufferHandle mltVisBufOutAlias = kage::alias(_initData.meshletVisBuffer);
    kage::ImageHandle colorOutAlias = kage::alias(_initData.color);
    kage::ImageHandle depthOutAlias = kage::alias(_initData.depth);

    kage::bindBuffer(pass, _initData.meshDrawCmdBuffer
        , 0
        , kage::PipelineStageFlagBits::task_shader
        , kage::AccessFlagBits::shader_read);
    
    kage::bindBuffer(pass, _initData.meshBuffer
        , 1
        , kage::PipelineStageFlagBits::task_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.meshDrawBuffer
        , 2
        , kage::PipelineStageFlagBits::task_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.meshletBuffer
        , 3
        , kage::PipelineStageFlagBits::task_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.meshletDataBuffer
        , 4
        , kage::PipelineStageFlagBits::task_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.vtxBuffer
        , 5
        , kage::PipelineStageFlagBits::task_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.transformBuffer
        , 6
        , kage::PipelineStageFlagBits::task_shader
        , kage::AccessFlagBits::shader_read);

    kage::bindBuffer(pass, _initData.meshletVisBuffer
        , 7
        , kage::PipelineStageFlagBits::task_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , mltVisBufOutAlias);

    kage::SamplerHandle pyrSampler = kage::sampleImage(pass, _initData.pyramid
        , 8
        , kage::PipelineStageFlagBits::fragment_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::setIndirectBuffer(pass, _initData.meshDrawCmdCountBuffer, 4, 1, 0);

    kage::setAttachmentOutput(pass, _initData.color, 0, colorOutAlias);
    kage::setAttachmentOutput(pass, _initData.depth, 0, depthOutAlias);

    // set the data
    _meshShading.late = _late;

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
    _meshShading.color = _initData.color;
    _meshShading.depth = _initData.depth;

    _meshShading.meshletVisBufferOutAlias = mltVisBufOutAlias;
    _meshShading.colorOutAlias = colorOutAlias;
    _meshShading.depthOutAlias = depthOutAlias;
}

void prepareTaskSubmit(TaskSubmit& _taskSubmit, kage::BufferHandle _drawCmdBuf, kage::BufferHandle _drawCmdCntBuf, bool _late /*= false*/, bool _alphaPass /*= false*/)
{
    kage::ShaderHandle cs = kage::registShader("task_modify_late", "shaders/taskModify.comp.spv");

    kage::ProgramHandle prog = kage::registProgram("task_modify_late", { cs });
    
    kage::PassDesc desc{};
    desc.programId = prog.id;
    desc.queue = kage::PassExeQueue::compute;

    const char* passName = 
        _alphaPass 
        ? "task_modify_alpha"
        : _late 
            ? "task_modify_late" 
            : "task_modify_early";
    kage::PassHandle pass = kage::registPass(passName, desc);

    kage::BufferHandle drawCmdBufferOutAlias = kage::alias(_drawCmdBuf);

    kage::bindBuffer(pass, _drawCmdCntBuf
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read);
    
    kage::bindBuffer(pass, _drawCmdBuf
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , drawCmdBufferOutAlias);

    _taskSubmit.cs = cs;
    _taskSubmit.prog = prog;
    _taskSubmit.pass = pass;

    _taskSubmit.drawCmdBuffer = _drawCmdBuf;
    _taskSubmit.drawCmdCountBuffer = _drawCmdCntBuf;
    _taskSubmit.drawCmdBufferOutAlias = drawCmdBufferOutAlias;
}

void updateTaskSubmit(const TaskSubmit& _taskSubmit)
{
    taskSubmitRec(_taskSubmit);
}

void updateMeshShading(MeshShading& _meshShading, const Globals& _globals)
{
    _meshShading.globals = _globals;

    meshShadingRec(_meshShading);
}