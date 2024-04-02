
#include "vkz_ms_pip.h"
#include "demo_structs.h"
#include "scene.h"
#include "vkz_math.h"

#include "bx/readerwriter.h"


void meshShading_renderFunc(kage::CommandListI& _cmdList, const void* _data, uint32_t _size)
{
    VKZ_ZoneScopedC(kage::Color::cyan);

    bx::MemoryReader reader(_data, _size);

    MeshShading msd;
    bx::read(&reader, msd, nullptr);

    _cmdList.barrier(msd.color, kage::AccessFlagBits::color_attachment_write, kage::ImageLayout::color_attachment_optimal, kage::PipelineStageFlagBits::color_attachment_output);
    _cmdList.barrier(msd.depth, kage::AccessFlagBits::depth_stencil_attachment_write, kage::ImageLayout::depth_stencil_attachment_optimal, kage::PipelineStageFlagBits::late_fragment_tests);
    _cmdList.dispatchBarriers();

    _cmdList.beginRendering(msd.pass);

    kage::Viewport viewport = { 0.f, (float)msd.height, (float)msd.width, -(float)msd.height, 0.f, 1.f };
    kage::Rect2D scissor = { {0, 0}, {uint32_t(msd.width), uint32_t(msd.height)} };
    _cmdList.setViewPort(0, 1, &viewport);
    _cmdList.setScissorRect(0, 1, &scissor);

    _cmdList.pushConstants(msd.pass, &msd.globals, sizeof(GlobalsVKZ));

    kage::CommandListI::DescriptorSet descs[] = 
    { 
        {msd.meshDrawCmdBuffer}, 
        {msd.meshBuffer}, 
        {msd.meshDrawBuffer}, 
        {msd.meshletBuffer}, 
        {msd.meshletDataBuffer}, 
        {msd.vtxBuffer}, 
        {msd.transformBuffer}, 
        {msd.meshletVisBuffer}, 
        {msd.pyramid, msd.pyramidSampler}
    };

    _cmdList.pushDescriptorSetWithTemplate(msd.pass, descs, COUNTOF(descs));

    _cmdList.drawMeshTaskIndirect(msd.meshDrawCmdCountBuffer, 4, 1, 0);

    _cmdList.endRendering();

    _cmdList.barrier(msd.meshletVisBufferOutAlias, kage::AccessFlagBits::shader_write, kage::PipelineStageFlagBits::mesh_shader);
    _cmdList.barrier(msd.color, kage::AccessFlagBits::color_attachment_write, kage::ImageLayout::color_attachment_optimal, kage::PipelineStageFlagBits::color_attachment_output);
    _cmdList.barrier(msd.depth, kage::AccessFlagBits::depth_stencil_attachment_write, kage::ImageLayout::depth_stencil_attachment_optimal, kage::PipelineStageFlagBits::late_fragment_tests);
}

void prepareMeshShading(MeshShading& _meshShading, const Scene& _scene, uint32_t _width, uint32_t _height, const MeshShadingInitData _initData, bool _late /* = false*/)
{
    kage::ShaderHandle ms= kage::registShader("mesh_shader", "shaders/meshlet.mesh.spv");
    kage::ShaderHandle ts = kage::registShader("task_shader", "shaders/meshlet.task.spv");
    kage::ShaderHandle fs = kage::registShader("mesh_frag_shader", "shaders/meshlet.frag.spv");


    kage::ProgramHandle prog = kage::registProgram("mesh_prog", { ts, ms, fs }, sizeof(GlobalsVKZ));

    int pipelineSpecs[] = { _late, true };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));


    kage::PassDesc desc{};
    desc.programId = prog.id;
    desc.queue = kage::PassExeQueue::graphics;
    desc.pipelineConfig.depthCompOp = kage::CompareOp::greater;
    desc.pipelineConfig.enableDepthTest = true;
    desc.pipelineConfig.enableDepthWrite = true;

    desc.passConfig.colorLoadOp = _late ? kage::AttachmentLoadOp::dont_care : kage::AttachmentLoadOp::dont_care;
    desc.passConfig.colorStoreOp = kage::AttachmentStoreOp::store;
    desc.passConfig.depthLoadOp = _late ? kage::AttachmentLoadOp::dont_care : kage::AttachmentLoadOp::clear;
    desc.passConfig.depthStoreOp = kage::AttachmentStoreOp::store;

    desc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    desc.pipelineSpecData = (void*)pConst->data;

    const char* passName = _late ? "mesh_pass_late" : "mesh_pass_early";
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

    kage::SamplerHandle pyrSampler =  kage::sampleImage(pass, _initData.pyramid
        , 8
        , kage::PipelineStageFlagBits::fragment_shader
        , kage::ImageLayout::shader_read_only_optimal
        , kage::SamplerReductionMode::weighted_average);

    kage::setIndirectBuffer(pass, _initData.meshDrawCmdCountBuffer, 4, 1, 0);

    kage::setAttachmentOutput(pass, _initData.color, 0, colorOutAlias);
    kage::setAttachmentOutput(pass, _initData.depth, 0, depthOutAlias);

    // set the data
    _meshShading.width = _width;
    _meshShading.height = _height;

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

    _meshShading.meshletVisBuffer = _initData.meshletVisBuffer;
    _meshShading.pyramid = _initData.pyramid;
    _meshShading.color = _initData.color;
    _meshShading.depth = _initData.depth;

    _meshShading.meshletVisBufferOutAlias = mltVisBufOutAlias;
    _meshShading.colorOutAlias = colorOutAlias;
    _meshShading.depthOutAlias = depthOutAlias;

    const kage::Memory* mem = kage::alloc(sizeof(MeshShading));
    memcpy(mem->data, &_meshShading, mem->size);
    kage::setCustomRenderFunc(pass, meshShading_renderFunc, mem);
}

void prepareTaskSubmit(TaskSubmit& _taskSubmit, kage::BufferHandle _drawCmdBuf, kage::BufferHandle _drawCmdCntBuf, bool _late /*= false*/)
{
    kage::ShaderHandle cs = kage::registShader("task_modify_shader", "shaders/taskModify.comp.spv");

    kage::ProgramHandle prog = kage::registProgram("task_modify_prog", { cs });

    int pipelineSpecs[] = { _late, true };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));
    
    kage::PassDesc desc{};
    desc.programId = prog.id;
    desc.queue = kage::PassExeQueue::compute;

    desc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    desc.pipelineSpecData = (void*)pConst->data;

    const char* passName = _late ? "task_modify_pass_late" : "task_modify_pass_early";
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

void updateMeshShadingConstants(MeshShading& _meshShading, const GlobalsVKZ& _globals)
{
    _meshShading.globals = _globals;

    const kage::Memory* mem = kage::alloc(sizeof(MeshShading));
    memcpy(mem->data, &_meshShading, mem->size);
    kage::updateCustomRenderFuncData(_meshShading.pass, mem);
}