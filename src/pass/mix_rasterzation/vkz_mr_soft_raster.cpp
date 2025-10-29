#include "vkz_mr_soft_raster.h"
#include "vkz_pass.h"
#include "demo_structs.h"
#include "core/kage_math.h"

void recSoftRaster(const SoftRaster& _raster)
{
    kage::startRec(_raster.pass);

    // only clear images in the early render stage
    if(_raster.renderStage == RenderStage::early) {
        kage::ClearImage clearImgs[] =
        {
            { _raster.inColor,  Aspect::color, {kage::ClearColor { 0.f, 0.f, 0.f, 1.f }}},
            { _raster.inDepth,  Aspect::depth, {kage::ClearDepthStencil{0.f, 0}}},
            { _raster.u32depth, Aspect::color, {kage::ClearColor { 0u, 0u, 0u, 0u }}},
            { _raster.u32depth, Aspect::color, {kage::ClearColor { 0u, 0u, 0u, 0u }}}
        };

        kage::clearImages(clearImgs, COUNTOF(clearImgs));
    }
    
    vec2 res = vec2(float(_raster.width), float(_raster.height));

    const kage::Memory* mem = kage::alloc(sizeof(res));
    bx::memCopy(mem->data, &res, mem->size);
    // set constants
    kage::setConstants(mem);

    // bind resources
    kage::Binding binds[] =
    {
        { _raster.meshDrawBuf,      BindingAccess::read,    Stage::compute_shader },
        { _raster.transformBuf,     BindingAccess::read,    Stage::compute_shader },
        { _raster.vtxBuf,           BindingAccess::read,    Stage::compute_shader },
        { _raster.meshletBuf,       BindingAccess::read,    Stage::compute_shader },
        { _raster.meshletDataBuf,   BindingAccess::read,    Stage::compute_shader },
        { _raster.payloadBuf,       BindingAccess::read,    Stage::compute_shader },
        { _raster.payloadCntBuf,    BindingAccess::read,    Stage::compute_shader },
        { _raster.pyramid,          _raster.pyramidSamp,    Stage::compute_shader },
        { _raster.inColor,          0,                      Stage::compute_shader },
        { _raster.u32depth,         0,                      Stage::compute_shader },
        { _raster.inDepth,          0,                      Stage::compute_shader },
        { _raster.u32debugImg,      0,                      Stage::compute_shader },
    };

    kage::pushBindings(binds, COUNTOF(binds));

    kage::dispatchIndirect(_raster.payloadCntBuf, offsetof(IndirectDispatchCommand, x));
    kage::endRec();
}

void initSoftRaster(SoftRaster& _softRaster, const SoftRasterDataInit& _initData)
{
    kage::ShaderHandle cs = kage::registShader("soft_raster", "shader/soft_raster.comp.spv");

    kage::ProgramHandle prog = kage::registProgram("soft_raster", { cs }, sizeof(vec2));

    kage::PassDesc passDesc;
    passDesc.prog = prog;
    passDesc.queue = kage::PassExeQueue::compute;

    kage::PassHandle pass = kage::registPass("soft_raster", passDesc);

    kage::ImageDesc u32depthDesc;
    u32depthDesc.width = _initData.width;
    u32depthDesc.height = _initData.height;
    u32depthDesc.depth = 1;
    u32depthDesc.numLayers = 1;
    u32depthDesc.numMips = 1;
    u32depthDesc.format = kage::ResourceFormat::r32_uint;
    u32depthDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage;
    kage::ImageHandle u32depth = kage::registTexture("depth_u32", u32depthDesc, nullptr, kage::ResourceLifetime::non_transition);

    kage::ImageDesc u32debugDesc;
    u32debugDesc.width = _initData.width;
    u32debugDesc.height = _initData.height;
    u32debugDesc.depth = 1;
    u32debugDesc.numLayers = 1;
    u32debugDesc.numMips = 1;
    u32debugDesc.format = kage::ResourceFormat::r32_uint;
    u32debugDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage;
    kage::ImageHandle u32debug = kage::registTexture("debug_u32", u32debugDesc, nullptr, kage::ResourceLifetime::non_transition);


    // output images
    kage::ImageHandle outColor = kage::alias(_initData.color);
    kage::ImageHandle outDepth = kage::alias(_initData.depth);
    kage::ImageHandle outU32Depth = kage::alias(u32depth);
    kage::ImageHandle outU32Debug = kage::alias(u32debug);

    kage::setIndirectBuffer(pass, _initData.payloadCntBuf);

    kage::bindBuffer(pass
        , _initData.meshDrawBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        , _initData.transformBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        , _initData.vtxBuf
        , Stage::compute_shader
        , Access::shader_read
    );
    
    kage::bindBuffer(pass
        , _initData.meshletBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::bindBuffer(pass
        ,_initData.meshletDataBuf
        , Stage::compute_shader
        , Access::shader_read
    );


    kage::bindBuffer(pass
        , _initData.payloadBuf
        , Stage::compute_shader
        , Access::shader_read
    );


    kage::bindBuffer(pass
        , _initData.payloadCntBuf
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::SamplerHandle samp = kage::sampleImage(pass, _initData.pyramid
        , Stage::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::bindImage(pass
        , _initData.color
        , Stage::compute_shader
        , Access::shader_read | Access::shader_write
        , kage::ImageLayout::general
        , outColor
    );

    kage::bindImage(pass
        , u32depth
        , Stage::compute_shader
        , Access::shader_read | Access::shader_write
        , kage::ImageLayout::general
        , outU32Depth
    );

    kage::bindImage(pass
        , _initData.depth
        , Stage::compute_shader
        , Access::shader_read | Access::shader_write
        , kage::ImageLayout::general
        , outDepth
    );

    kage::bindImage(pass
        , u32debug
        , Stage::compute_shader
        , Access::shader_write
        , kage::ImageLayout::general
        , outU32Debug
    );

    _softRaster.pass = pass;
    _softRaster.cs = cs;
    _softRaster.prog = prog;

    _softRaster.payloadBuf = _initData.payloadBuf;
    _softRaster.payloadCntBuf = _initData.payloadCntBuf;

    _softRaster.meshDrawBuf = _initData.meshDrawBuf;
    _softRaster.transformBuf = _initData.transformBuf;
    _softRaster.vtxBuf = _initData.vtxBuf;
    _softRaster.meshletBuf = _initData.meshletBuf;
    _softRaster.meshletDataBuf = _initData.meshletDataBuf;

    _softRaster.inColor = _initData.color;
    _softRaster.inDepth = _initData.depth;
    _softRaster.u32depth = u32depth;
    _softRaster.u32depthOutAlias = outU32Depth;
    _softRaster.colorOutAlias = outColor;
    _softRaster.depthOutAlias = outDepth;

    _softRaster.pyramid = _initData.pyramid;
    _softRaster.pyramidSamp = samp;

    _softRaster.width = _initData.width;
    _softRaster.height = _initData.height;

    _softRaster.renderStage = _initData.renderStage;

    // debug image
    _softRaster.u32debugImg = u32debug;
    _softRaster.u32debugImgOutAlias = u32debug;
}

void updateSoftRaster(SoftRaster& _softRaster)
{
    recSoftRaster(_softRaster);
}

