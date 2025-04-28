#pragma once

#include "kage_math.h"
#include "vkz_rc2d.h"
#include "config.h"

using Binding = kage::Binding;
using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;
using LoadOp = kage::AttachmentLoadOp;
using StoreOp = kage::AttachmentStoreOp;


struct Rc2dData
{
    uint32_t width;
    uint32_t height;

    uint32_t c0_dRes;
    uint32_t nCascades;
};

struct Rc2DBuildData
{
    Rc2dData rc;
    float tamx[8];
};


void initRc2DBuild(Rc2DBuild& _rc, const Rc2dData& _init)
{
    // build the cascade image
    kage::ShaderHandle cs = kage::registShader("build_rc2d", "shaders/rc2d_build.comp.spv");
    kage::ProgramHandle program = kage::registProgram("build_rc2d", { cs }, sizeof(Rc2DBuildData));

    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::compute;

    kage::PassHandle pass = kage::registPass("build_rc2d", passDesc);

    kage::ImageDesc imgDesc{};
    imgDesc.width = _init.width;
    imgDesc.height = _init.height;
    imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    imgDesc.depth = 1;
    imgDesc.numLayers = _init.nCascades;
    imgDesc.numMips = 1;
    imgDesc.type = kage::ImageType::type_2d;
    imgDesc.viewType = kage::ImageViewType::type_2d_array;
    imgDesc.usage = kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::storage | kage::ImageUsageFlagBits::sampled;
    imgDesc.layout = kage::ImageLayout::general;

    kage::ImageHandle img = kage::registTexture("rc2d", imgDesc, nullptr, kage::ResourceLifetime::non_transition);
    kage::ImageHandle outAlias = kage::alias(img);

    kage::bindImage(pass, img
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , outAlias
    );

    _rc.cs = cs;
    _rc.prog = program;
    _rc.pass = pass;

    _rc.rcImage = img;
    _rc.rcImageOutAlias = outAlias;
}

void initRc2DMerge(Rc2DMerge& _rc, const Rc2dData _init, kage::ImageHandle _inRc)
{
    kage::ShaderHandle cs = kage::registShader("merge_rc2d", "shaders/rc2d_merge.comp.spv");
    kage::ProgramHandle program = kage::registProgram("merge_rc2d", { cs }, sizeof(Rc2dData));
    
    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("merge_rc2d", passDesc);
    
    kage::ImageDesc imgDesc{};
    imgDesc.width = _init.width;
    imgDesc.height = _init.height;
    imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    imgDesc.depth = 1;
    imgDesc.numLayers = 1;
    imgDesc.numMips = 1;
    imgDesc.type = kage::ImageType::type_2d;
    imgDesc.viewType = kage::ImageViewType::type_2d_array;
    imgDesc.usage = kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::storage | kage::ImageUsageFlagBits::sampled;
    imgDesc.layout = kage::ImageLayout::general;
    
    kage::ImageHandle img = kage::registTexture("rc2d_merged", imgDesc, nullptr, kage::ResourceLifetime::non_transition);

    kage::SamplerHandle linearSamp = kage::sampleImage(pass, _inRc
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle nearedSamp = kage::sampleImage(pass, img
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    kage::ImageHandle outAlias = kage::alias(img);
    kage::bindImage(pass, img
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , outAlias
    );

    _rc.cs = cs;
    _rc.prog = program;
    _rc.pass = pass;

    _rc.in_rc = _inRc;
    _rc.linearSamp = linearSamp;
    _rc.nearedSamp = nearedSamp;
    _rc.mergedCas = img;
    _rc.mergedCasOutAlias = outAlias;
}

void initRc2DUse(Rc2DUse& _rc, const Rc2dData& _init, kage::ImageHandle _inMergedRc)
{
    kage::ShaderHandle cs = kage::registShader("use_rc2d", "shaders/rc2d_use.comp.spv");
    kage::ProgramHandle program = kage::registProgram("use_rc2d", { cs }, sizeof(Rc2dData));
    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("use_rc2d", passDesc);
    kage::ImageDesc imgDesc{};
    
    imgDesc.width = _init.width;
    imgDesc.height = _init.height;
    imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    imgDesc.depth = 1;
    imgDesc.numLayers = 1;
    imgDesc.numMips = 1;
    imgDesc.type = kage::ImageType::type_2d;
    imgDesc.viewType = kage::ImageViewType::type_2d;
    imgDesc.usage = kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::storage | kage::ImageUsageFlagBits::sampled | kage::ImageUsageFlagBits::color_attachment | kage::ImageUsageFlagBits::transfer_src;
    imgDesc.layout = kage::ImageLayout::general;
    kage::ImageHandle img = kage::registTexture("rc2d_use", imgDesc, nullptr, kage::ResourceLifetime::non_transition);

    kage::SamplerHandle linearSamp = kage::sampleImage(pass, _inMergedRc
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle nearedSamp = kage::sampleImage(pass, img
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    kage::ImageHandle outAlias = kage::alias(img);
    kage::bindImage(pass, img
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , outAlias
    );

    _rc.cs = cs;
    _rc.prog = program;
    _rc.pass = pass;

    _rc.rt = img;

    _rc.mergedCas = _inMergedRc;
    _rc.linearSamp = linearSamp;
    _rc.nearedSamp = nearedSamp;

    _rc.rtOutAlias = outAlias;
}

void initRc2D(Rc2D& _rc, uint32_t _w, uint32_t _h, uint32_t _rayRes, uint32_t _casCount)
{
    Rc2dData initData = { _w, _h, _rayRes, _casCount };

    initRc2DBuild(_rc.build, initData);
    initRc2DMerge(_rc.merge, initData, _rc.build.rcImageOutAlias);
    initRc2DUse(_rc.use, initData, _rc.merge.mergedCasOutAlias);
}


void recRc2DBuild(const Rc2DBuild& _rc, const Rc2dData& _init)
{
    kage::startRec(_rc.pass);

    const kage::Memory* mem = kage::alloc(sizeof(Rc2dData));
    memcpy(mem->data, &_init, mem->size);
    kage::setConstants(mem);


    kage::Binding pushBinds[] =
    {
        {_rc.rcImage, 0, Stage::compute_shader},
    };
    kage::pushBindings(pushBinds, COUNTOF(pushBinds));
    kage::dispatch(_init.width, _init.height, _init.nCascades);
    kage::endRec();
}

void recRc2DMerge(const Rc2DMerge& _rc, const Rc2dData& _init)
{
    kage::startRec(_rc.pass);
    const kage::Memory* mem = kage::alloc(sizeof(Rc2dData));
    memcpy(mem->data, &_init, mem->size);
    kage::setConstants(mem);


    kage::Binding pushBinds[] = {
        {_rc.in_rc,     _rc.linearSamp, Stage::compute_shader},
        {_rc.mergedCas, _rc.nearedSamp, Stage::compute_shader},
        {_rc.mergedCas, 0,              Stage::compute_shader},
    };
    kage::pushBindings(pushBinds, COUNTOF(pushBinds));
    kage::dispatch(_init.width, _init.height, 1);
    kage::endRec();
}

void recRc2DUse(const Rc2DUse& _rc, const Rc2dData& _init)
{
    kage::startRec(_rc.pass);
    const kage::Memory* mem = kage::alloc(sizeof(Rc2dData));
    memcpy(mem->data, &_init, mem->size);
    kage::setConstants(mem);


    kage::Binding pushBinds[] = {
        {_rc.mergedCas, _rc.linearSamp, Stage::compute_shader},
        {_rc.rt,     0, Stage::compute_shader},
    };
    kage::pushBindings(pushBinds, COUNTOF(pushBinds));
    kage::dispatch(_init.width, _init.height, 1);
    kage::endRec();
}


void updateRc2DBuild(Rc2DBuild& _rc, const Rc2dData& _data)
{
}

void updateRc2DMerge(Rc2DMerge& _rc, const Rc2dData& _data)
{
}

void updateRc2DUse(Rc2DUse& _rc, const Rc2dData& _data)
{
}

void updateRc2D(Rc2D& _rc, const Rc2dData& _data)
{
    updateRc2DBuild(_rc.build, _data);
    updateRc2DMerge(_rc.merge, _data);
    updateRc2DUse(_rc.use, _data);
}

void updateRc2D(Rc2D& _rc, uint32_t _w, uint32_t _h, uint32_t _rayRes, uint32_t _casCount)
{
    Rc2dData data = { _w, _h, _rayRes, _casCount };

    updateRc2D(_rc, data);

    recRc2DBuild(_rc.build, data);
    recRc2DMerge(_rc.merge, data);
    recRc2DUse(_rc.use, data);
}

