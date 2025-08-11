#include "vkz_soft_rasterization.h"
#include "demo_structs.h"
#include "core/kage_math.h"

using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;
using LoadOp = kage::AttachmentLoadOp;
using StoreOp = kage::AttachmentStoreOp;
using Aspect = kage::ImageAspectFlagBits::Enum;


void recSoftRasterization(const SoftRasterization& _raster)
{
    kage::startRec(_raster.pass);

    kage::ClearAttachment atts[] =
    {
        { _raster.inColor,  Aspect::color, {kage::ClearColor { 0.f, 0.f, 0.f, 1.f }}},
        { _raster.inDepth,  Aspect::depth, {kage::ClearDepthStencil{0.f, 0}}},
    };

    kage::clearAttachments(atts, COUNTOF(atts));
    
    vec2 res = vec2(float(_raster.width), float(_raster.height));

    const kage::Memory* mem = kage::alloc(sizeof(res));
    bx::memCopy(mem->data, &res, mem->size);
    // set constants
    kage::setConstants(mem);

    // bind resources
    
    kage::Binding binds[] =
    {
        { _raster.inColor,         0,                  Stage::compute_shader },
        { _raster.inDepth,         0,                  Stage::compute_shader },
    };

    kage::pushBindings(binds, COUNTOF(binds));

    // dispatch compute shader
    kage::dispatch(_raster.width, _raster.height, 1);
    kage::endRec();
}

void initSoftRasterization(SoftRasterization& _softRaster, const SoftRasterizationDataInit& _initData)
{
    kage::ShaderHandle cs = kage::registShader("soft_rasterization", "shader/soft_rasterization.comp.spv");

    kage::ProgramHandle prog = kage::registProgram("soft_rasterization", { cs }, sizeof(vec2));

    kage::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;

    kage::PassHandle pass = kage::registPass("soft_rasterization", passDesc);

    // output images
    kage::ImageHandle outColor = kage::alias(_initData.color);
    kage::ImageHandle outDepth = kage::alias(_initData.depth);

    kage::bindImage(pass
        , _initData.color
        , Stage::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , outColor
    );

    kage::bindImage(pass
        , _initData.depth
        , Stage::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , outDepth
    );

    _softRaster.pass = pass;
    _softRaster.cs = cs;
    _softRaster.prog = prog;

    _softRaster.width = _initData.width;
    _softRaster.height = _initData.height;

    _softRaster.inColor = _initData.color;
    _softRaster.inDepth = _initData.depth;
    _softRaster.colorOutAlias = outColor;
    _softRaster.depthOutAlias = outDepth;
}

void updateSoftRasterization(SoftRasterization& _softRaster)
{
    recSoftRasterization(_softRaster);
}

