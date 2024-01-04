#include "vkz.h"
/*
void right()
{
    vkz::init();

    vkz::ImageDesc colorDesc;
    colorDesc.format = vkz::ResourceFormat::r8g8b8a8_snorm;
    colorDesc.width = 1024;
    colorDesc.height = 1024;
    colorDesc.depth = 1;
    colorDesc.arrayLayers = 1;
    colorDesc.mips = 1;
    vkz::RenderTargetHandle color = vkz::registRenderTarget("color", colorDesc);
    vkz::RenderTargetHandle color_late = vkz::aliasRenderTarget(color);

    vkz::ImageDesc depthDesc;
    depthDesc.format = vkz::ResourceFormat::d32;
    depthDesc.width = 1024;
    depthDesc.height = 1024;
    depthDesc.depth = 1;
    depthDesc.arrayLayers = 1;
    depthDesc.mips = 1;
    vkz::DepthStencilHandle depth = vkz::registDepthStencil("depth", depthDesc);
    vkz::DepthStencilHandle depth_late = vkz::aliasDepthStencil(depth);

    vkz::ImageDesc outputDesc;
    outputDesc.format = vkz::ResourceFormat::r8g8b8a8_snorm;
    outputDesc.width = 1024;
    outputDesc.height = 1024;
    outputDesc.depth = 1;
    outputDesc.arrayLayers = 1;
    outputDesc.mips = 1;
    vkz::RenderTargetHandle output = vkz::registRenderTarget("output", outputDesc);
    vkz::setPresentImage(output);

    vkz::ImageDesc pyramidDesc;
    pyramidDesc.format = vkz::ResourceFormat::r32_sfloat;
    pyramidDesc.width = 1024;
    pyramidDesc.height = 1024;
    pyramidDesc.depth = 1;
    pyramidDesc.arrayLayers = 1;
    pyramidDesc.mips = 8;
    vkz::TextureHandle pyramid = vkz::registTexture("pyramid", pyramidDesc);
    vkz::TextureHandle pyramid_lastFrame = vkz::aliasTexture(pyramid);

    vkz::BufferDesc mltBufDesc;
    mltBufDesc.size = 1024 * 1024 * 1024;
    vkz::BufferHandle mltBuf = vkz::registBuffer("mlt", mltBufDesc);

    vkz::BufferDesc mvisBufDesc;
    mvisBufDesc.size = 1024 * 1024 * 1024;
    vkz::BufferHandle mvisBuf = vkz::registBuffer("mvis", mltBufDesc);
    vkz::BufferHandle mvisLastFrame = vkz::aliasBuffer(mvisBuf);
    vkz::BufferHandle mvisLate = vkz::aliasBuffer(mvisBuf);

    vkz::BufferDesc idxBufDesc;
    idxBufDesc.size = 1024 * 1024 * 1024;
    vkz::BufferHandle idxBuf = vkz::registBuffer("idx", idxBufDesc);

    vkz::BufferDesc vtxBufDesc;
    vtxBufDesc.size = 1024 * 1024 * 1024;
    vkz::BufferHandle vtxBuf = vkz::registBuffer("vtx", vtxBufDesc);

    vkz::BufferDesc cmdBufDesc;
    cmdBufDesc.size = 1024 * 1024 * 1;
    vkz::BufferHandle cmdBuf = vkz::registBuffer("cmd", cmdBufDesc);
    vkz::BufferHandle cmdBuf_late = vkz::aliasBuffer(cmdBuf);

    vkz::PassDesc cull_a_desc;
    cull_a_desc.queue = vkz::PassExeQueue::compute;
    vkz::PassHandle cull_a = vkz::registPass("cull_a", cull_a_desc);

    vkz::PassDesc draw_a_desc;
    draw_a_desc.queue = vkz::PassExeQueue::graphics;
    vkz::PassHandle draw_a = vkz::registPass("draw_a", draw_a_desc);

    vkz::PassDesc py_desc;
    py_desc.queue = vkz::PassExeQueue::compute;
    vkz::PassHandle py_pass = vkz::registPass("py_pass", py_desc);

    vkz::PassDesc cull_b_desc;
    cull_b_desc.queue = vkz::PassExeQueue::compute;
    vkz::PassHandle cull_b = vkz::registPass("cull_b", cull_b_desc);

    vkz::PassDesc draw_b_desc;
    draw_b_desc.queue = vkz::PassExeQueue::graphics;
    vkz::PassHandle draw_b = vkz::registPass("draw_b", draw_b_desc);

    vkz::PassDesc output_desc;
    output_desc.queue = vkz::PassExeQueue::graphics;
    vkz::PassHandle out_pass = vkz::registPass("output", output_desc);

    // set multi-frame resource
    vkz::setMultiFrameBuffer({ mvisBuf });
    vkz::setMultiFrameTexture({ pyramid });

    // set resource for passes
    // cull_a
    vkz::passReadTextures(cull_a, { pyramid_lastFrame });
    vkz::passReadBuffers(cull_a, { mltBuf, mvisLastFrame });
    vkz::passWriteBuffers(cull_a, { cmdBuf , mvisBuf });

    // draw_a
    vkz::passReadBuffers(draw_a, { cmdBuf });
    vkz::passReadTextures(draw_a, { pyramid_lastFrame });
    vkz::passWriteRTs(draw_a, { color });
    vkz::passWriteDSs(draw_a, { depth });

    // pyramid
    vkz::passReadDSs(py_pass, { depth });
    vkz::passWriteTextures(py_pass, { pyramid });

    // cull_b
    vkz::passReadTextures(cull_b, { pyramid });
    vkz::passReadBuffers(cull_b, { mltBuf, mvisBuf });
    vkz::passWriteBuffers(cull_b, { cmdBuf_late , mvisLate});

    // draw_b
    vkz::passReadBuffers(draw_b, { cmdBuf_late });
    vkz::passReadTextures(draw_b, { pyramid });
    vkz::passWriteRTs(draw_b, { color_late });
    vkz::passWriteDSs(draw_b, { depth_late });

    // output
    vkz::passReadRTs(out_pass, { color_late });
    vkz::passWriteRTs(out_pass, { output });

    vkz::loop();

    vkz::shutdown();
}

void wrong()
{
    vkz::init();

    vkz::ImageDesc colorDesc;
    colorDesc.format = vkz::ResourceFormat::r8g8b8a8_snorm;
    colorDesc.width = 1024;
    colorDesc.height = 1024;
    colorDesc.depth = 1;
    colorDesc.arrayLayers = 1;
    colorDesc.mips = 1;
    vkz::RenderTargetHandle color = vkz::registRenderTarget("color", colorDesc);

    vkz::ImageDesc depthDesc;
    depthDesc.format = vkz::ResourceFormat::d32;
    depthDesc.width = 1024;
    depthDesc.height = 1024;
    depthDesc.depth = 1;
    depthDesc.arrayLayers = 1;
    depthDesc.mips = 1;
    vkz::DepthStencilHandle depth = vkz::registDepthStencil("depth", depthDesc);

    vkz::ImageDesc outputDesc;
    outputDesc.format = vkz::ResourceFormat::r8g8b8a8_snorm;
    outputDesc.width = 1024;
    outputDesc.height = 1024;
    outputDesc.depth = 1;
    outputDesc.arrayLayers = 1;
    outputDesc.mips = 1;
    vkz::RenderTargetHandle output = vkz::registRenderTarget("output", outputDesc);

    vkz::ImageDesc pyramidDesc;
    pyramidDesc.format = vkz::ResourceFormat::r32_sfloat;
    pyramidDesc.width = 1024;
    pyramidDesc.height = 1024;
    pyramidDesc.depth = 1;
    pyramidDesc.arrayLayers = 1;
    pyramidDesc.mips = 8;
    vkz::TextureHandle pyramid = vkz::registTexture("pyramid", pyramidDesc);

    vkz::BufferDesc mltBufDesc;
    mltBufDesc.size = 1024 * 1024 * 1024;
    vkz::BufferHandle mltBuf = vkz::registBuffer("idx", mltBufDesc);

    vkz::BufferDesc idxBufDesc;
    idxBufDesc.size = 1024 * 1024 * 1024;
    vkz::BufferHandle idxBuf = vkz::registBuffer("idx", idxBufDesc);

    vkz::BufferDesc vtxBufDesc;
    vtxBufDesc.size = 1024 * 1024 * 1024;
    vkz::BufferHandle vtxBuf = vkz::registBuffer("vtx", vtxBufDesc);

    vkz::BufferDesc cmdBufDesc;
    cmdBufDesc.size = 1024 * 1024 * 1;
    vkz::BufferHandle cmdBuf = vkz::registBuffer("cmd", cmdBufDesc);

    vkz::PassDesc cull_a_desc;
    cull_a_desc.queue = vkz::PassExeQueue::compute;
    vkz::PassHandle cull_a = vkz::registPass("cull_a", cull_a_desc);

    vkz::PassDesc draw_a_desc;
    draw_a_desc.queue = vkz::PassExeQueue::graphics;
    vkz::PassHandle draw_a = vkz::registPass("draw_a", draw_a_desc);

    vkz::PassDesc cull_b_desc;
    cull_b_desc.queue = vkz::PassExeQueue::compute;
    vkz::PassHandle cull_b = vkz::registPass("cull_b", cull_b_desc);

    vkz::PassDesc draw_b_desc;
    draw_b_desc.queue = vkz::PassExeQueue::graphics;
    vkz::PassHandle draw_b = vkz::registPass("draw_b", draw_b_desc);

    vkz::PassDesc py_desc;
    py_desc.queue = vkz::PassExeQueue::compute;
    vkz::PassHandle py_pass = vkz::registPass("py_pass", py_desc);

    vkz::PassDesc output_desc;
    output_desc.queue = vkz::PassExeQueue::graphics;
    vkz::PassHandle out_pass = vkz::registPass("output", output_desc);

    // set resource for passes
    // cull_a
    vkz::passReadTextures(cull_a, { pyramid });
    vkz::passReadBuffers(cull_a, { mltBuf });
    vkz::passWriteBuffers(cull_a, { cmdBuf });

    // draw_a
    vkz::passReadBuffers(draw_a, { cmdBuf, idxBuf, vtxBuf });
    vkz::passWriteRTs(draw_a, { color });
    vkz::passWriteDSs(draw_a, { depth });

    // pyramid
    vkz::passReadDSs(py_pass, { depth });
    vkz::passWriteTextures(py_pass, { pyramid });

    // cull_b
    vkz::passReadTextures(cull_b, { pyramid });
    vkz::passReadBuffers(cull_b, { mltBuf });
    vkz::passWriteBuffers(cull_b, { cmdBuf });

    // draw_b
    vkz::passReadBuffers(draw_b, { cmdBuf, idxBuf, vtxBuf });
    vkz::passReadTextures(draw_b, { pyramid });
    vkz::passWriteRTs(draw_b, { color });
    vkz::passWriteDSs(draw_b, { depth });

    // output
    vkz::passReadRTs(out_pass, { color });
    vkz::passWriteRTs(out_pass, { output });


    vkz::loop();

    vkz::shutdown();
}
*/
void triangle()
{
    vkz::init();

    vkz::ImageDesc rtDesc;
    rtDesc.width = 2560;
    rtDesc.height = 1440;
    rtDesc.depth = 1;
    rtDesc.arrayLayers = 1;
    rtDesc.mips = 1;
    rtDesc.usage = vkz::ImageUsageFlagBits::color_attachment
        | vkz::ImageUsageFlagBits::transfer_src;

    vkz::ImageHandle color = vkz::registRenderTarget("color", rtDesc);


    vkz::ShaderHandle vs = vkz::registShader("color_vert_shader", "shaders/triangle.vert.spv");
    vkz::ShaderHandle fs = vkz::registShader("color_frag_shader", "shaders/triangle.frag.spv");

    vkz::ProgramHandle program = vkz::registProgram("color_prog", {vs, fs});

    vkz::PassDesc result;
    result.programId = program.id;
    result.queue = vkz::PassExeQueue::graphics;
    result.pipelineConfig.depthCompOp = vkz::CompareOp::greater;
    result.pipelineConfig.enableDepthTest = true;
    result.pipelineConfig.enableDepthWrite = true;

    vkz::BufferDesc dummyBufDesc;
    dummyBufDesc.size = 32;
    dummyBufDesc.usage = vkz::BufferUsageFlagBits::storage;
    vkz::BufferHandle dummyBuf = vkz::registBuffer("dummy", dummyBufDesc);

    vkz::PassHandle pass = vkz::registPass("result", result);

    {
        vkz::ResInteractDesc interact = {};
        interact.binding = 0;
        interact.stage = vkz::PipelineStageFlagBits::vertex_shader;
        interact.access = vkz::AccessFlagBits::shader_read;
        vkz::passReadBuffer(pass, dummyBuf, interact);
    }

    {
        vkz::ResInteractDesc interact = {};
        interact.binding = 0;
        interact.stage = vkz::PipelineStageFlagBits::color_attachment_output;
        interact.access = vkz::AccessFlagBits::none;
        interact.layout = vkz::ImageLayout::color_attachment_optimal;
        vkz::passWriteImage(pass, color, interact);
    }

    vkz::setPresentImage(color);

    vkz::loop();

    vkz::shutdown();
}

void DemoMain()
{
    triangle();
}