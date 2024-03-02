
#include "vkz_ms_pip.h"
#include "demo_structs.h"
#include "scene.h"
#include "memory_operation.h"
#include "math.h"


void meshShading_renderFunc(vkz::CommandListI& _cmdList, const void* _data, uint32_t _size)
{
    VKZ_ZoneScopedC(vkz::Color::cyan);

    vkz::MemoryReader reader(_data, _size);

    MeshShading msd;
    vkz::read(&reader, msd);

    _cmdList.barrier(msd.color, vkz::AccessFlagBits::color_attachment_write, vkz::ImageLayout::color_attachment_optimal, vkz::PipelineStageFlagBits::color_attachment_output);
    _cmdList.barrier(msd.depth, vkz::AccessFlagBits::depth_stencil_attachment_write, vkz::ImageLayout::depth_stencil_attachment_optimal, vkz::PipelineStageFlagBits::late_fragment_tests);
    _cmdList.dispatchBarriers();

    _cmdList.beginRendering(msd.pass);

    vkz::Viewport viewport = { 0.f, (float)msd.height, (float)msd.width, -(float)msd.height, 0.f, 1.f };
    vkz::Rect2D scissor = { {0, 0}, {uint32_t(msd.width), uint32_t(msd.height)} };
    _cmdList.setViewPort(0, 1, &viewport);
    _cmdList.setScissorRect(0, 1, &scissor);

    _cmdList.pushConstants(msd.pass, &msd.globals, sizeof(GlobalsVKZ));

    vkz::CommandListI::DescriptorSet descs[] = 
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

    _cmdList.barrier(msd.meshletVisBufferOutAlias, vkz::AccessFlagBits::shader_write, vkz::PipelineStageFlagBits::mesh_shader);
    _cmdList.barrier(msd.color, vkz::AccessFlagBits::color_attachment_write, vkz::ImageLayout::color_attachment_optimal, vkz::PipelineStageFlagBits::color_attachment_output);
    _cmdList.barrier(msd.depth, vkz::AccessFlagBits::depth_stencil_attachment_write, vkz::ImageLayout::depth_stencil_attachment_optimal, vkz::PipelineStageFlagBits::late_fragment_tests);
}

void prepareMeshShading(MeshShading& _meshShading, const Scene& _scene, uint32_t _width, uint32_t _height, const MeshShadingInitData _initData, bool _late /* = false*/)
{
    vkz::ShaderHandle ms= vkz::registShader("mesh_shader", "shaders/meshlet.mesh.spv");
    vkz::ShaderHandle ts = vkz::registShader("task_shader", "shaders/meshlet.task.spv");
    vkz::ShaderHandle fs = vkz::registShader("mesh_frag_shader", "shaders/meshlet.frag.spv");


    vkz::ProgramHandle prog = vkz::registProgram("mesh_prog", { ts, ms, fs }, sizeof(GlobalsVKZ));

    int pipelineSpecs[] = { _late, true };

    const vkz::Memory* pConst = vkz::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));


    vkz::PassDesc desc{};
    desc.programId = prog.id;
    desc.queue = vkz::PassExeQueue::graphics;
    desc.pipelineConfig.depthCompOp = vkz::CompareOp::greater;
    desc.pipelineConfig.enableDepthTest = true;
    desc.pipelineConfig.enableDepthWrite = true;

    desc.passConfig.colorLoadOp = _late ? vkz::AttachmentLoadOp::load : vkz::AttachmentLoadOp::clear;
    desc.passConfig.colorStoreOp = vkz::AttachmentStoreOp::store;
    desc.passConfig.depthLoadOp = _late ? vkz::AttachmentLoadOp::load : vkz::AttachmentLoadOp::clear;
    desc.passConfig.depthStoreOp = vkz::AttachmentStoreOp::store;

    desc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    desc.pipelineSpecData = (void*)pConst->data;

    const char* passName = _late ? "mesh_pass_late" : "mesh_pass_early";
    vkz::PassHandle pass = vkz::registPass(passName, desc);

    vkz::BufferHandle mltVisBufOutAlias = vkz::alias(_initData.meshletVisBuffer);
    vkz::ImageHandle colorOutAlias = vkz::alias(_initData.color);
    vkz::ImageHandle depthOutAlias = vkz::alias(_initData.depth);

    vkz::bindBuffer(pass, _initData.meshDrawCmdBuffer
        , 0
        , vkz::PipelineStageFlagBits::task_shader
        , vkz::AccessFlagBits::shader_read);
    
    vkz::bindBuffer(pass, _initData.meshBuffer
        , 1
        , vkz::PipelineStageFlagBits::task_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.meshDrawBuffer
        , 2
        , vkz::PipelineStageFlagBits::task_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.meshletBuffer
        , 3
        , vkz::PipelineStageFlagBits::task_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.meshletDataBuffer
        , 4
        , vkz::PipelineStageFlagBits::task_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.vtxBuffer
        , 5
        , vkz::PipelineStageFlagBits::task_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.transformBuffer
        , 6
        , vkz::PipelineStageFlagBits::task_shader
        , vkz::AccessFlagBits::shader_read);

    vkz::bindBuffer(pass, _initData.meshletVisBuffer
        , 7
        , vkz::PipelineStageFlagBits::task_shader
        , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
        , mltVisBufOutAlias);

    vkz::SamplerHandle pyrSampler =  vkz::sampleImage(pass, _initData.pyramid
        , 8
        , vkz::PipelineStageFlagBits::fragment_shader
        , vkz::ImageLayout::shader_read_only_optimal
        , vkz::SamplerReductionMode::weighted_average);

    vkz::setIndirectBuffer(pass, _initData.meshDrawCmdCountBuffer, 4, 1, 0);

    vkz::setAttachmentOutput(pass, _initData.color, 0, colorOutAlias);
    vkz::setAttachmentOutput(pass, _initData.depth, 0, depthOutAlias);

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

    const vkz::Memory* mem = vkz::alloc(sizeof(MeshShading));
    memcpy(mem->data, &_meshShading, mem->size);
    vkz::setCustomRenderFunc(pass, meshShading_renderFunc, mem);
}

void prepareTaskSubmit(TaskSubmit& _taskSubmit, vkz::BufferHandle _drawCmdBuf, vkz::BufferHandle _drawCmdCntBuf, bool _late /*= false*/)
{
    vkz::ShaderHandle cs = vkz::registShader("task_modify_shader", "shaders/taskModify.comp.spv");

    vkz::ProgramHandle prog = vkz::registProgram("task_modify_prog", { cs });

    int pipelineSpecs[] = { _late, true };

    const vkz::Memory* pConst = vkz::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));
    
    vkz::PassDesc desc{};
    desc.programId = prog.id;
    desc.queue = vkz::PassExeQueue::compute;

    desc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    desc.pipelineSpecData = (void*)pConst->data;

    const char* passName = _late ? "task_modify_pass_late" : "task_modify_pass_early";
    vkz::PassHandle pass = vkz::registPass(passName, desc);

    vkz::BufferHandle drawCmdBufferOutAlias = vkz::alias(_drawCmdBuf);

    vkz::bindBuffer(pass, _drawCmdCntBuf
        , 0
        , vkz::PipelineStageFlagBits::compute_shader
        , vkz::AccessFlagBits::shader_read);
    
    vkz::bindBuffer(pass, _drawCmdBuf
        , 1
        , vkz::PipelineStageFlagBits::compute_shader
        , vkz::AccessFlagBits::shader_write
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

    const vkz::Memory* mem = vkz::alloc(sizeof(MeshShading));
    memcpy(mem->data, &_meshShading, mem->size);
    vkz::updateCustomRenderFuncData(_meshShading.pass, mem);
}