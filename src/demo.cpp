#include "vkz.h"

void right()
{
    vkz::init();

    vkz::ImageDesc colorDesc;
    colorDesc.format = vkz::TextureFormat::RGBA8S;
    colorDesc.x = 1024;
    colorDesc.y = 1024;
    colorDesc.z = 1;
    colorDesc.layers = 1;
    colorDesc.mips = 1;
    vkz::RenderTargetHandle color = vkz::registRenderTarget("color", colorDesc);
    vkz::RenderTargetHandle color_late = vkz::aliasRenderTarget(color);

    vkz::ImageDesc depthDesc;
    depthDesc.format = vkz::TextureFormat::D32;
    depthDesc.x = 1024;
    depthDesc.y = 1024;
    depthDesc.z = 1;
    depthDesc.layers = 1;
    depthDesc.mips = 1;
    vkz::DepthStencilHandle depth = vkz::registDepthStencil("depth", depthDesc);
    vkz::DepthStencilHandle depth_late = vkz::aliasDepthStencil(depth);

    vkz::ImageDesc outputDesc;
    outputDesc.format = vkz::TextureFormat::RGBA8S;
    outputDesc.x = 1024;
    outputDesc.y = 1024;
    outputDesc.z = 1;
    outputDesc.layers = 1;
    outputDesc.mips = 1;
    vkz::RenderTargetHandle output = vkz::registRenderTarget("output", outputDesc);
    vkz::setResultRenderTarget(output);

    vkz::ImageDesc pyramidDesc;
    pyramidDesc.format = vkz::TextureFormat::R32;
    pyramidDesc.x = 1024;
    pyramidDesc.y = 1024;
    pyramidDesc.z = 1;
    pyramidDesc.layers = 1;
    pyramidDesc.mips = 8;
    vkz::TextureHandle pyramid = vkz::registTexture("pyramid", pyramidDesc);
    vkz::TextureHandle pyramid_lastFrame = vkz::aliasTexture(pyramid);

    vkz::BufferDesc mltBufDesc;
    mltBufDesc.size = 1024 * 1024 * 1024;
    mltBufDesc.memPropFlags = 0;
    vkz::BufferHandle mltBuf = vkz::registBuffer("mlt", mltBufDesc);

    vkz::BufferDesc mvisBufDesc;
    mvisBufDesc.size = 1024 * 1024 * 1024;
    mvisBufDesc.memPropFlags = 0;
    vkz::BufferHandle mvisBuf = vkz::registBuffer("mvis", mltBufDesc);
    vkz::BufferHandle mvisLastFrame = vkz::aliasBuffer(mvisBuf);
    vkz::BufferHandle mvisLate = vkz::aliasBuffer(mvisBuf);

    vkz::BufferDesc idxBufDesc;
    idxBufDesc.size = 1024 * 1024 * 1024;
    idxBufDesc.memPropFlags = 0;
    vkz::BufferHandle idxBuf = vkz::registBuffer("idx", idxBufDesc);

    vkz::BufferDesc vtxBufDesc;
    vtxBufDesc.size = 1024 * 1024 * 1024;
    vtxBufDesc.memPropFlags = 0;
    vkz::BufferHandle vtxBuf = vkz::registBuffer("vtx", vtxBufDesc);

    vkz::BufferDesc cmdBufDesc;
    cmdBufDesc.size = 1024 * 1024 * 1;
    cmdBufDesc.memPropFlags;
    vkz::BufferHandle cmdBuf = vkz::registBuffer("cmd", cmdBufDesc);
    vkz::BufferHandle cmdBuf_late = vkz::aliasBuffer(cmdBuf);

    vkz::PassDesc cull_a_desc;
    cull_a_desc.queue = vkz::PassExeQueue::AsyncCompute0;
    vkz::PassHandle cull_a = vkz::registPass("cull_a", cull_a_desc);

    vkz::PassDesc draw_a_desc;
    draw_a_desc.queue = vkz::PassExeQueue::Graphics;
    vkz::PassHandle draw_a = vkz::registPass("draw_a", draw_a_desc);

    vkz::PassDesc py_desc;
    py_desc.queue = vkz::PassExeQueue::AsyncCompute0;
    vkz::PassHandle py_pass = vkz::registPass("py_pass", py_desc);

    vkz::PassDesc cull_b_desc;
    cull_b_desc.queue = vkz::PassExeQueue::AsyncCompute0;
    vkz::PassHandle cull_b = vkz::registPass("cull_b", cull_b_desc);

    vkz::PassDesc draw_b_desc;
    draw_b_desc.queue = vkz::PassExeQueue::Graphics;
    vkz::PassHandle draw_b = vkz::registPass("draw_b", draw_b_desc);

    vkz::PassDesc output_desc;
    output_desc.queue = vkz::PassExeQueue::Graphics;
    vkz::PassHandle out_pass = vkz::registPass("output", output_desc);

    // set resource for passes
    // cull_a
    vkz::readTextures(cull_a, { pyramid_lastFrame });
    vkz::readBuffers(cull_a, { mltBuf, mvisLastFrame });
    vkz::writeBuffers(cull_a, { cmdBuf , mvisBuf });

    // draw_a
    vkz::readBuffers(draw_a, { cmdBuf });
    vkz::readTextures(draw_a, { pyramid_lastFrame });
    vkz::writeRenderTargets(draw_a, { color });
    vkz::writeDepthStencils(draw_a, { depth });

    // pyramid
    vkz::readDepthStencils(py_pass, { depth });
    vkz::writeTextures(py_pass, { pyramid });

    // cull_b
    vkz::readTextures(cull_b, { pyramid });
    vkz::readBuffers(cull_b, { mltBuf, mvisBuf });
    vkz::writeBuffers(cull_b, { cmdBuf_late , mvisLate});

    // draw_b
    vkz::readBuffers(draw_b, { cmdBuf_late });
    vkz::readTextures(draw_b, { pyramid });
    vkz::writeRenderTargets(draw_b, { color_late });
    vkz::writeDepthStencils(draw_b, { depth_late });

    // output
    vkz::readRenderTargets(out_pass, { color_late });
    vkz::writeRenderTargets(out_pass, { output });

    vkz::update();

    vkz::shutdown();
}

void wrong()
{
    vkz::init();

    const vkz::Memory* mem = vkz::alloc(1024 * 1024 * 1024);
    // TODO: load shader data here
    // ...

    vkz::ShaderHandle sha = vkz::registShader("sha", mem);

    vkz::ImageDesc colorDesc;
    colorDesc.format = vkz::TextureFormat::RGBA8S;
    colorDesc.x = 1024;
    colorDesc.y = 1024;
    colorDesc.z = 1;
    colorDesc.layers = 1;
    colorDesc.mips = 1;
    vkz::RenderTargetHandle color = vkz::registRenderTarget("color", colorDesc);

    vkz::ImageDesc depthDesc;
    depthDesc.format = vkz::TextureFormat::D32;
    depthDesc.x = 1024;
    depthDesc.y = 1024;
    depthDesc.z = 1;
    depthDesc.layers = 1;
    depthDesc.mips = 1;
    vkz::DepthStencilHandle depth = vkz::registDepthStencil("depth", depthDesc);

    vkz::ImageDesc outputDesc;
    outputDesc.format = vkz::TextureFormat::RGBA8S;
    outputDesc.x = 1024;
    outputDesc.y = 1024;
    outputDesc.z = 1;
    outputDesc.layers = 1;
    outputDesc.mips = 1;
    vkz::RenderTargetHandle output = vkz::registRenderTarget("output", outputDesc);

    vkz::ImageDesc pyramidDesc;
    pyramidDesc.format = vkz::TextureFormat::D32;
    pyramidDesc.x = 1024;
    pyramidDesc.y = 1024;
    pyramidDesc.z = 1;
    pyramidDesc.layers = 1;
    pyramidDesc.mips = 8;
    vkz::TextureHandle pyramid = vkz::registTexture("pyramid", pyramidDesc);

    vkz::BufferDesc mltBufDesc;
    mltBufDesc.size = 1024 * 1024 * 1024;
    mltBufDesc.memPropFlags = 0;
    vkz::BufferHandle mltBuf = vkz::registBuffer("idx", mltBufDesc);

    vkz::BufferDesc idxBufDesc;
    idxBufDesc.size = 1024 * 1024 * 1024;
    idxBufDesc.memPropFlags = 0;
    vkz::BufferHandle idxBuf = vkz::registBuffer("idx", idxBufDesc);

    vkz::BufferDesc vtxBufDesc;
    vtxBufDesc.size = 1024 * 1024 * 1024;
    vtxBufDesc.memPropFlags = 0;
    vkz::BufferHandle vtxBuf = vkz::registBuffer("vtx", vtxBufDesc);

    vkz::BufferDesc cmdBufDesc;
    cmdBufDesc.size = 1024 * 1024 * 1;
    cmdBufDesc.memPropFlags;
    vkz::BufferHandle cmdBuf = vkz::registBuffer("cmd", cmdBufDesc);

    vkz::PassDesc cull_a_desc;
    cull_a_desc.queue = vkz::PassExeQueue::AsyncCompute0;
    vkz::PassHandle cull_a = vkz::registPass("cull_a", cull_a_desc);

    vkz::PassDesc draw_a_desc;
    draw_a_desc.queue = vkz::PassExeQueue::Graphics;
    vkz::PassHandle draw_a = vkz::registPass("draw_a", draw_a_desc);

    vkz::PassDesc cull_b_desc;
    cull_b_desc.queue = vkz::PassExeQueue::AsyncCompute0;
    vkz::PassHandle cull_b = vkz::registPass("cull_b", cull_b_desc);

    vkz::PassDesc draw_b_desc;
    draw_b_desc.queue = vkz::PassExeQueue::Graphics;
    vkz::PassHandle draw_b = vkz::registPass("draw_b", draw_b_desc);

    vkz::PassDesc py_desc;
    py_desc.queue = vkz::PassExeQueue::AsyncCompute0;
    vkz::PassHandle py_pass = vkz::registPass("py_pass", py_desc);

    vkz::PassDesc output_desc;
    output_desc.queue = vkz::PassExeQueue::Graphics;
    vkz::PassHandle out_pass = vkz::registPass("output", output_desc);

    // set resource for passes
    // cull_a
    vkz::readTextures(cull_a, { pyramid });
    vkz::readBuffers(cull_a, { mltBuf });
    vkz::writeBuffers(cull_a, { cmdBuf });

    // draw_a
    vkz::readBuffers(draw_a, { cmdBuf, idxBuf, vtxBuf });
    vkz::writeRenderTargets(draw_a, { color });
    vkz::writeDepthStencils(draw_a, { depth });

    // pyramid
    vkz::readDepthStencils(py_pass, { depth });
    vkz::writeTextures(py_pass, { pyramid });

    // cull_b
    vkz::readTextures(cull_b, { pyramid });
    vkz::readBuffers(cull_b, { mltBuf });
    vkz::writeBuffers(cull_b, { cmdBuf });

    // draw_b
    vkz::readBuffers(draw_b, { cmdBuf, idxBuf, vtxBuf });
    vkz::readTextures(draw_b, { pyramid });
    vkz::writeRenderTargets(draw_b, { color });
    vkz::writeDepthStencils(draw_b, { depth });

    // output
    vkz::readRenderTargets(out_pass, { color });
    vkz::writeRenderTargets(out_pass, { output });


    vkz::update();
    // 

    vkz::shutdown();
}

void DemoMain()
{
    right();
}