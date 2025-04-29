#pragma once

#include "kage_math.h"
#include "vkz_rc2d.h"
#include "config.h"

using Binding = kage::Binding;
using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;
using LoadOp = kage::AttachmentLoadOp;
using StoreOp = kage::AttachmentStoreOp;

constexpr uint32_t c_maxCascads = 8;

struct Rc2dData
{
    uint32_t width;
    uint32_t height;

    uint32_t c0_dRes;
    uint32_t nCascades;

    float mpx;
    float mpy;
};

struct alignas(16) Rc2dMergeData
{
    Rc2dData rc;
    
    uint32_t lv;

    uint32_t arrow;
};

struct alignas(16) Rc2dUseData
{
    Rc2dData rc;
    
    uint32_t lv;
    uint32_t stage;

    uint32_t arrow;
};


void initRc2DBuild(Rc2DBuild& _rc, const Rc2dData& _init)
{
    // build the cascade image
    kage::ShaderHandle cs = kage::registShader("build_rc2d", "shaders/rc2d_build.comp.spv");
    kage::ProgramHandle program = kage::registProgram("build_rc2d", { cs }, sizeof(Rc2dData));

    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::compute;

    kage::PassHandle pass = kage::registPass("build_rc2d", passDesc);

    kage::ImageDesc imgDesc{};
    imgDesc.width = _init.width;
    imgDesc.height = _init.height;
    imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    imgDesc.depth = 1;
    imgDesc.numLayers = c_maxCascads;
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

void initRc2DMerge(Rc2DMerge& _rc, const Rc2dData _init, kage::ImageHandle _inRc, bool _ray)
{
    const char* name = _ray ? "rc2d_merge_ray" : "rc2d_merge_probe";

    kage::ShaderHandle cs = kage::registShader(name, "shaders/rc2d_merge.comp.spv");
    kage::ProgramHandle program = kage::registProgram(name, { cs }, sizeof(Rc2dMergeData));
    
    int pipelineSpecs[] = { _ray };
    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecData = (void*)pConst->data;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);

    kage::PassHandle pass = kage::registPass(name, passDesc);

    kage::ImageDesc imgDesc{};
    imgDesc.width = _ray ? _init.width : _init.width / _init.c0_dRes + 1;
    imgDesc.height = _ray ? _init.height : _init.height / _init.c0_dRes + 1;
    imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    imgDesc.depth = 1;
    imgDesc.numLayers = _ray ? c_maxCascads : 1;
    imgDesc.numMips = 1;
    imgDesc.type = kage::ImageType::type_2d;
    imgDesc.viewType = kage::ImageViewType::type_2d_array;
    imgDesc.usage = kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::storage | kage::ImageUsageFlagBits::sampled;
    imgDesc.layout = kage::ImageLayout::general;
    
    kage::ImageHandle img = kage::registTexture(name, imgDesc, nullptr, kage::ResourceLifetime::non_transition);

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
    _rc.mergedInterval = img;
    _rc.mergedCasOutAlias = outAlias;
}


void initRc2DUse(Rc2DUse& _rc, const Rc2dData& _init, kage::ImageHandle _inMergedRc, kage::ImageHandle _inRc, kage::ImageHandle _inMergedProbe)
{
    kage::ShaderHandle cs = kage::registShader("use_rc2d", "shaders/rc2d_use.comp.spv");
    kage::ProgramHandle program = kage::registProgram("use_rc2d", { cs }, sizeof(Rc2dUseData));
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

    kage::sampleImage(pass, _inMergedProbe
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

    _rc.rc = _inRc;
    _rc.mergedInterval = _inMergedRc;
    _rc.mergedProbe = _inMergedProbe;
    _rc.linearSamp = linearSamp;
    _rc.nearedSamp = nearedSamp;

    _rc.rtOutAlias = outAlias;
}

void initRc2D(Rc2D& _rc, const Rc2dInfo& _info)
{
    Rc2dData data = { _info.width, _info.height, _info.c0_dRes, _info.nCascades, _info.mpx, _info.mpy };

    initRc2DBuild(_rc.build, data);
    initRc2DMerge(_rc.mergeInterval, data, _rc.build.rcImageOutAlias, true);
    initRc2DMerge(_rc.mergeProbe, data, _rc.mergeInterval.mergedCasOutAlias, false);
    initRc2DUse(_rc.use, data, _rc.mergeInterval.mergedCasOutAlias, _rc.build.rcImageOutAlias, _rc.mergeProbe.mergedCasOutAlias);

    _rc.width = _info.width;
    _rc.height = _info.height;
    _rc.nCascades = _info.nCascades;
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

void recRc2DMerge(const Rc2DMerge& _rc, const Rc2dData& _init, bool _ray)
{
    kage::startRec(_rc.pass);

    uint32_t startCasN1 = _ray ? _init.nCascades - 1 : 1;
    // start from nCascades - 2
    // because the final lv needs no futher merge so just skip it and start from the n-2
    for (uint32_t  ii = startCasN1; ii > 0 ; --ii)
    {
        uint32_t currLv = ii - 1;

        Rc2dMergeData mergeInterval;
        mergeInterval.rc = _init;
        mergeInterval.lv = currLv;

        const kage::Memory* mem = kage::alloc(sizeof(Rc2dMergeData));
        memcpy(mem->data, &mergeInterval, mem->size);
        kage::setConstants(mem);

        kage::ImageHandle scratchImg = currLv == (_init.nCascades - 2) ? _rc.in_rc : _rc.mergedInterval;

        kage::Binding pushBinds[] = {
            {_rc.in_rc,     0,      Stage::compute_shader},
            {scratchImg,    0,      Stage::compute_shader},
            {_rc.mergedInterval, 0,      Stage::compute_shader},
        };
        kage::pushBindings(pushBinds, COUNTOF(pushBinds));

        uint32_t w = _ray ? _init.width : _init.width / _init.c0_dRes + 1;
        uint32_t h = _ray ? _init.height : _init.height / _init.c0_dRes + 1;
        kage::dispatch(w, h, 1);
    }
    kage::endRec();
}

void recRc2DUse(const Rc2DUse& _rc, const Rc2dData& _data, const Dbg_Rc2d& _dbg)
{
    kage::startRec(_rc.pass);

    Rc2dUseData use;
    use.rc = _data;
    use.lv = _dbg.lv;
    use.stage = _dbg.stage;
    use.arrow = _dbg.showArrow ? 1 : 0;

    const kage::Memory* mem = kage::alloc(sizeof(Rc2dUseData));
    memcpy(mem->data, &use, mem->size);
    kage::setConstants(mem);

    kage::Binding pushBinds[] = {
        {_rc.rc,                _rc.nearedSamp, Stage::compute_shader},
        {_rc.mergedInterval,    _rc.linearSamp, Stage::compute_shader},
        {_rc.mergedProbe,       _rc.linearSamp, Stage::compute_shader},
        {_rc.rt,                0,              Stage::compute_shader},
    };

    kage::pushBindings(pushBinds, COUNTOF(pushBinds));
    kage::dispatch(_data.width, _data.height, 1);
    
    kage::endRec();
}


void updateRc2DBuild(Rc2DBuild& _rc, const Rc2dData& _data)
{
    kage::updateImage(_rc.rcImage, _data.width, _data.height, c_maxCascads);
}

void updateRc2DMerge(Rc2DMerge& _rc, const Rc2dData& _data, bool _ray)
{
    uint32_t w = _ray ? _data.width : _data.width / _data.c0_dRes + 1;
    uint32_t h = _ray ? _data.height : _data.height / _data.c0_dRes + 1;
    uint32_t d = _ray ? c_maxCascads : 1;
    kage::updateImage(_rc.mergedInterval, w, h, d);
}

void updateRc2DUse(Rc2DUse& _rc, const Rc2dData& _data)
{
    kage::updateImage(_rc.rt, _data.width, _data.height, 1);
}

void updateRc2D(Rc2D& _rc, const Rc2dData& _data)
{
    if (_rc.width != _data.width || _rc.height != _data.height)
    {
        updateRc2DBuild(_rc.build, _data);
        updateRc2DMerge(_rc.mergeInterval, _data, true);
        updateRc2DMerge(_rc.mergeProbe, _data, false);
        updateRc2DUse(_rc.use, _data);

        _rc.width = _data.width;
        _rc.height = _data.height;
    }
}

void updateRc2D(Rc2D& _rc, const Rc2dInfo& _info, const Dbg_Rc2d& _dbg)
{
    Rc2dData data = { _info.width, _info.height, _info.c0_dRes, _info.nCascades, _info.mpx, _info.mpy };

    updateRc2D(_rc, data);

    recRc2DBuild(_rc.build, data);
    recRc2DMerge(_rc.mergeInterval, data, true);
    recRc2DMerge(_rc.mergeProbe, data, false);
    recRc2DUse(_rc.use, data, _dbg);
}

