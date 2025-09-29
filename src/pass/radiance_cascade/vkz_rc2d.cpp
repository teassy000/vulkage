#include "vkz_rc2d.h"
#include "vkz_pass.h"

#include "core/kage_math.h"
#include "core/config.h"


constexpr uint32_t c_maxCascads = 8;

struct Rc2dData
{
    uint32_t w;
    uint32_t h;

    uint32_t c0_dRes;
    uint32_t nCascades;

    float mpx;
    float mpy;

    float c0_rLen;
};


struct alignas(16) Rc2dMergeData
{
    Rc2dData rc;
    
    uint32_t lv;

    uint32_t flags;
};

struct alignas(16) Rc2dUseData
{
    Rc2dData rc;
    
    uint32_t lv;
    uint32_t stage;

    uint32_t flags;
};

uvec2 calcRCRes(uint32_t _scW, uint32_t _scH, uint32_t _nCas, uint32_t _c0dRes)
{
    vec2 rcRes = vec2(_scW, _scH);
    
    uint32_t factor = 1 << (_nCas - 1);
    uint32_t cn_dRes = _c0dRes * factor;

    vec2 cn_probeCount = glm::ceil(rcRes / vec2(float(cn_dRes)));
    uvec2 outRes = cn_probeCount * vec2(float(cn_dRes));

    return outRes;
}

void initRc2DBuild(Rc2DBuild& _rc, const uvec2 _res)
{
    // build the cascade image
    kage::ShaderHandle cs = kage::registShader("build_rc2d", "shader/rc2d_build.comp.spv");
    kage::ProgramHandle program = kage::registProgram("build_rc2d", { cs }, sizeof(Rc2dData));

    kage::PassDesc passDesc{};
    passDesc.prog = program;
    passDesc.queue = kage::PassExeQueue::compute;

    kage::PassHandle pass = kage::registPass("build_rc2d", passDesc);

    kage::ImageDesc imgDesc{};
    imgDesc.width = _res.x;
    imgDesc.height = _res.y;
    imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    imgDesc.depth = 1;
    imgDesc.numLayers = c_maxCascads;
    imgDesc.numMips = 1;
    imgDesc.type = kage::ImageType::type_2d;
    imgDesc.viewType = kage::ImageViewType::type_2d_array;
    imgDesc.usage = ImgUsage::transfer_dst | ImgUsage::storage | ImgUsage::sampled;
    imgDesc.layout = kage::ImageLayout::general;

    kage::ImageHandle img = kage::registTexture("rc2d", imgDesc, nullptr, kage::ResourceLifetime::non_transition);
    kage::ImageHandle outAlias = kage::alias(img);

    kage::bindImage(pass, img
        , Stage::compute_shader
        , Access::shader_write
        , kage::ImageLayout::general
        , outAlias
    );

    _rc.cs = cs;
    _rc.prog = program;
    _rc.pass = pass;

    _rc.rcImage = img;
    _rc.rcImageOutAlias = outAlias;
}

void initRc2DMerge(Rc2DMerge& _rc, const uvec2 _init, uint32_t _c0dRes, kage::ImageHandle _inRc, bool _ray)
{
    const char* name = _ray ? "rc2d_merge_ray" : "rc2d_merge_probe";

    kage::ShaderHandle cs = kage::registShader(name, "shader/rc2d_merge.comp.spv");
    kage::ProgramHandle program = kage::registProgram(name, { cs }, sizeof(Rc2dMergeData));
    
    int pipelineSpecs[] = { _ray, true };
    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc{};
    passDesc.prog = program;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecData = (void*)pConst->data;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);

    kage::PassHandle pass = kage::registPass(name, passDesc);

    kage::ImageDesc imgDesc{};
    imgDesc.width = _ray ? _init.x : _init.x / _c0dRes;
    imgDesc.height = _ray ? _init.y : _init.y / _c0dRes;
    imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    imgDesc.depth = 1;
    imgDesc.numLayers = _ray ? c_maxCascads : 1;
    imgDesc.numMips = 1;
    imgDesc.type = kage::ImageType::type_2d;
    imgDesc.viewType = kage::ImageViewType::type_2d_array;
    imgDesc.usage = ImgUsage::transfer_dst | ImgUsage::storage | ImgUsage::sampled;
    imgDesc.layout = kage::ImageLayout::general;
    
    kage::ImageHandle img = kage::registTexture(name, imgDesc, nullptr, kage::ResourceLifetime::non_transition);

    kage::SamplerHandle linearSamp = kage::sampleImage(pass, _inRc
        , Stage::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle nearedSamp = kage::sampleImage(pass, img
        , Stage::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    kage::ImageHandle outAlias = kage::alias(img);
    kage::bindImage(pass, img
        , Stage::compute_shader
        , Access::shader_write
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

void initRc2DUse(Rc2DUse& _rc, uvec2 _res, kage::ImageHandle _inMergedRc, kage::ImageHandle _inRc, kage::ImageHandle _inMergedProbe)
{
    kage::ShaderHandle cs = kage::registShader("use_rc2d", "shader/rc2d_use.comp.spv");
    kage::ProgramHandle program = kage::registProgram("use_rc2d", { cs }, sizeof(Rc2dUseData));
    kage::PassDesc passDesc{};
    passDesc.prog = program;
    passDesc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("use_rc2d", passDesc);
    kage::ImageDesc imgDesc{};
    
    imgDesc.width = _res.x;
    imgDesc.height = _res.y;
    imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    imgDesc.depth = 1;
    imgDesc.numLayers = 1;
    imgDesc.numMips = 1;
    imgDesc.type = kage::ImageType::type_2d;
    imgDesc.viewType = kage::ImageViewType::type_2d;
    imgDesc.usage = ImgUsage::transfer_dst | ImgUsage::storage | ImgUsage::sampled | ImgUsage::color_attachment | ImgUsage::transfer_src;
    imgDesc.layout = kage::ImageLayout::general;
    kage::ImageHandle img = kage::registTexture("rc2d_use", imgDesc, nullptr, kage::ResourceLifetime::non_transition);

    kage::SamplerHandle linearSamp = kage::sampleImage(pass, _inMergedRc
        , Stage::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle nearedSamp = kage::sampleImage(pass, img
        , Stage::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    kage::sampleImage(pass, _inMergedProbe
        , Stage::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    kage::ImageHandle outAlias = kage::alias(img);
    kage::bindImage(pass, img
        , Stage::compute_shader
        , Access::shader_write
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
    uvec2 rcRes = calcRCRes(_info.rt_width, _info.rt_height, _info.nCascades, _info.c0_dRes);
    uvec2 rtRes = uvec2(_info.rt_width, _info.rt_height);

    initRc2DBuild(_rc.build, rcRes);
    initRc2DMerge(_rc.mergeInterval, rcRes, _info.c0_dRes, _rc.build.rcImageOutAlias, true);
    initRc2DMerge(_rc.mergeProbe, rcRes, _info.c0_dRes, _rc.mergeInterval.mergedCasOutAlias, false);
    initRc2DUse(_rc.use, rtRes, _rc.mergeInterval.mergedCasOutAlias, _rc.build.rcImageOutAlias, _rc.mergeProbe.mergedCasOutAlias);

    _rc.rt_width = rtRes.x;
    _rc.rt_height = rtRes.y;
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
    kage::dispatch(_init.w, _init.h, _init.nCascades);
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

        uint32_t w = _ray ? _init.w : _init.w / _init.c0_dRes;
        uint32_t h = _ray ? _init.h : _init.h / _init.c0_dRes;
        kage::dispatch(w, h, 1);
    }
    kage::endRec();
}

void recRc2DUse(const Rc2DUse& _rc, const Rc2dData& _data, const uvec2 _res, const Dbg_Rc2d& _dbg)
{
    kage::startRec(_rc.pass);

    Rc2dUseData use;
    use.rc = _data;
    use.lv = _dbg.lv;
    use.stage = _dbg.stage;

    uint32_t flags = 0;
    if (_dbg.showArrow)
        flags |= 1 << 0;

    if (_dbg.show_c0_Border)
        flags |= 1 << 1;


    use.flags = flags;

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
    kage::dispatch(_res.x, _res.y, 1);
    
    kage::endRec();
}


void updateRc2DBuild(Rc2DBuild& _rc, const uvec2 _res, uint32_t _nCascades)
{
    kage::updateImage(_rc.rcImage, _res.x, _res.y, _nCascades);
}

void updateRc2DMerge(Rc2DMerge& _rc, const uvec2 _res, uint32_t _c0dRes, uint32_t _nCascades, bool _ray)
{
    uint32_t w = _ray ? _res.x : _res.x / _c0dRes;
    uint32_t h = _ray ? _res.y : _res.y / _c0dRes;
    kage::updateImage(_rc.mergedInterval, w, h, _nCascades);
}

void updateRc2DUse(Rc2DUse& _rc, const uvec2 _res)
{
    kage::updateImage(_rc.rt, _res.x, _res.y, 1);
}

void updateRc2D(Rc2D& _rc, const Rc2dData& _data, const uvec2 _res)
{
    if (_rc.rt_width != _res.x || _rc.rt_height != _res.y)
    {
        uvec2 rcRes = uvec2(_data.w, _data.h);

        updateRc2DBuild(_rc.build, rcRes, c_maxCascads);
        updateRc2DMerge(_rc.mergeInterval, rcRes, _data.c0_dRes, c_maxCascads, true);
        updateRc2DMerge(_rc.mergeProbe, rcRes, _data.c0_dRes, 1, false);
        updateRc2DUse(_rc.use, _res);

        _rc.rt_width = _res.x;
        _rc.rt_height = _res.y;
    }
}

void updateRc2D(Rc2D& _rc, const Rc2dInfo& _info, const Dbg_Rc2d& _dbg)
{
    uvec2 rcRes = calcRCRes(_info.rt_width, _info.rt_height, _info.nCascades, _info.c0_dRes);
    Rc2dData data = { rcRes.x, rcRes.y, _info.c0_dRes, _info.nCascades, _info.mpx, _info.mpy, _dbg.c0_rLen };
    uvec2 rtRes = uvec2(_info.rt_width, _info.rt_height);

    updateRc2D(_rc, data, rtRes);

    recRc2DBuild(_rc.build, data);
    recRc2DMerge(_rc.mergeInterval, data, true);
    recRc2DMerge(_rc.mergeProbe, data, false);
    recRc2DUse(_rc.use, data, rtRes, _dbg);
}

