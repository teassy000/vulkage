#include "vkz_radiance_cascade.h"
#include "vkz_pass.h"
#include "core/config.h"
#include "assets/mesh.h"

struct RCBuildInit
{
    GBuffer g_buffer;
    kage::ImageHandle depth;
    kage::ImageHandle skybox;

    BRX_UserResources brx;
};

struct RCMergeInit
{
    kage::ImageHandle radianceCascade;
    kage::ImageHandle skybox;

    uint32_t currCas;
};

uint32_t getMaxCascadesCount()
{
    uint32_t lvCount = 0;
    while (true) {
        if ((kage::k_rclv0_probeSideCount >> lvCount) == 0)
            break;
        ++lvCount;
    }

    return lvCount - 1;
}

void prepareRCbuild(RadianceCascadeBuild& _rc, const RCBuildInit& _init)
{
    // build the cascade image
    kage::ShaderHandle cs = kage::registShader("build_cascade", "shader/rc_build_cascade.comp.spv");
    kage::ProgramHandle program = kage::registProgram("build_cascade", { cs }, sizeof(RadianceCascadesConfig));

    kage::PassDesc passDesc{};
    passDesc.prog = program;
    passDesc.queue = kage::PassExeQueue::compute;

    kage::PassHandle pass = kage::registPass("build_cascade", passDesc);

    uint32_t texelSideCount = kage::k_rclv0_probeSideCount * kage::k_rclv0_rayGridSideCount;
    uint32_t imgLayers = kage::k_rclv0_probeSideCount * 2 - 1;

    kage::ImageDesc imgDesc{};
    imgDesc.width = texelSideCount;
    imgDesc.height = texelSideCount;
    imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    imgDesc.depth = 1;
    imgDesc.numLayers = imgLayers;
    imgDesc.numMips = 1;
    imgDesc.type = kage::ImageType::type_2d;
    imgDesc.viewType = kage::ImageViewType::type_2d_array;
    imgDesc.usage = ImgUsage::transfer_dst | ImgUsage::storage | ImgUsage::sampled;
    imgDesc.layout = kage::ImageLayout::general;

    kage::ImageHandle img = kage::registTexture("cascade", imgDesc, nullptr, kage::ResourceLifetime::non_transition);
    kage::ImageHandle outAlias = kage::alias(img);

    // transform buffer
    kage::BufferDesc transDesc{};
    transDesc.size = sizeof(RadianceCascadesTransform);
    transDesc.usage = BufUsage::uniform | BufUsage::transfer_dst;
    kage::BufferHandle trans = kage::registBuffer("rc_trans", transDesc);

    kage::bindBuffer(pass, trans
        , Stage::compute_shader
        , Access::shader_read
    );

    kage::SamplerHandle albedoSamp = kage::sampleImage(pass, _init.g_buffer.albedo
        , Stage::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle normSamp = kage::sampleImage(pass, _init.g_buffer.normal
        , Stage::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle wpSamp = kage::sampleImage(pass, _init.g_buffer.worldPos
        , Stage::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle emmiSamp = kage::sampleImage(pass, _init.g_buffer.emissive
        , Stage::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle depthSamp = kage::sampleImage(pass, _init.depth
        , Stage::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle skySamp = kage::sampleImage(pass, _init.skybox
        , Stage::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::bindImage(pass, img
        , Stage::compute_shader
        , Access::shader_write
        , kage::ImageLayout::general
        , outAlias
    );

    kage::SamplerHandle brxAtlasSamp = kage::sampleImage(pass, _init.brx.sdfAtlas
        , Stage::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    _rc.pass = pass;
    _rc.program = program;
    _rc.cs = cs;

    _rc.g_buffer = _init.g_buffer;
    _rc.g_bufferSamplers.albedo = albedoSamp;
    _rc.g_bufferSamplers.normal = normSamp;
    _rc.g_bufferSamplers.worldPos = wpSamp;
    _rc.g_bufferSamplers.emissive = emmiSamp;
    _rc.inSkybox = _init.skybox;
    _rc.skySampler = skySamp;

    _rc.trans = trans;
    _rc.inDepth = _init.depth;
    _rc.depthSampler = depthSamp;


    // brx input
    memcpy(&_rc.brx, &_init.brx, sizeof(BRX_UserResources));
    _rc.brxAtlasSamp = brxAtlasSamp;

    _rc.cascadeImg = img;
    _rc.radCascdOutAlias = outAlias;
}

void updateRCBuild(RadianceCascadeBuild& _rc, const Dbg_RadianceCascades& _dbg, const TransformData& _trans)
{
    // update trans buf
    RadianceCascadesTransform trans;

    if (_dbg.followCam) {
        _rc.cameraPos = _trans.cameraPos;
    }

    if (!_dbg.pauseUpdate) {
        _rc.view = _trans.view;
        _rc.proj = _trans.proj;
    }

    trans.cameraPos = _rc.cameraPos;
    trans.view = _rc.view;
    trans.proj = _rc.proj;

    const kage::Memory* memTransform = kage::alloc(sizeof(RadianceCascadesTransform));
    memcpy_s(memTransform->data, memTransform->size, &trans, sizeof(RadianceCascadesTransform));
    kage::updateBuffer(_rc.trans, memTransform);
}

void recRCBuild(const RadianceCascadeBuild& _rc, const Dbg_RadianceCascades& _dbg)
{
    kage::startRec(_rc.pass);

    uint32_t cascadeCount = glm::min(kage::k_rclv0_cascadeLv, _dbg.cascadeCount);
    uint32_t endLv = _dbg.startCascade + cascadeCount;

    float rayStartLen = 0.f;
    for (uint32_t ii = 0; ii < endLv; ++ii)
    {
        uint32_t    level_factor    = uint32_t(1 << ii);
        uint32_t    prob_sideCount  = kage::k_rclv0_probeSideCount / level_factor;
        uint32_t    prob_count      = uint32_t(pow(prob_sideCount, 3));
        uint32_t    ray_sideCount   = kage::k_rclv0_rayGridSideCount * level_factor;
        uint32_t    ray_count       = ray_sideCount * ray_sideCount; // each probe has a 2d grid of rays
        float prob_sideLen = _dbg.totalRadius * 2.f / float(prob_sideCount);
        float rayEndLen =  (ii == endLv - 1) ? 200.f : length(vec3(prob_sideLen)) * .5f;

        RadianceCascadesConfig config;
        config.probe_sideCount = prob_sideCount;
        config.ray_gridSideCount = ray_sideCount;
        config.level = ii;
        config.layerOffset = (kage::k_rclv0_probeSideCount - prob_sideCount) * 2;
        config.rayStartLength = rayStartLen;
        config.rayEndLength = rayEndLen;
        config.probeSideLen = prob_sideLen;
        config.radius = _dbg.totalRadius;

        config.brx_tmin = _dbg.brx_tmin;
        config.brx_tmax = _dbg.brx_tmax;

        config.cx = _dbg.probePosOffset[0];
        config.cy = _dbg.probePosOffset[1];
        config.cz = _dbg.probePosOffset[2];

        uint32_t offset = 2 * kage::k_brixelizerCascadeCount;
        config.brx_offset = _dbg.brx_offset;
        config.brx_startCas = bx::min(_dbg.brx_startCas, _dbg.brx_endCas);
        config.brx_endCas = bx::max(_dbg.brx_startCas, _dbg.brx_endCas);
        config.brx_sdfEps = _dbg.brx_sdfEps;

        config.debug_idx_type = _dbg.idx_type;
        config.debug_color_type = _dbg.color_type;

        const kage::Memory* mem = kage::alloc(sizeof(RadianceCascadesConfig));
        memcpy(mem->data, &config, mem->size);

        kage::setConstants(mem);

        rayStartLen = rayEndLen;

        kage::Binding pushBinds[] =
        {
            {_rc.trans,                 BindingAccess::read,            Stage::compute_shader},
            {_rc.g_buffer.albedo,       _rc.g_bufferSamplers.albedo,    Stage::compute_shader},
            {_rc.g_buffer.normal,       _rc.g_bufferSamplers.normal,    Stage::compute_shader},
            {_rc.g_buffer.worldPos,     _rc.g_bufferSamplers.worldPos,  Stage::compute_shader},
            {_rc.g_buffer.emissive,     _rc.g_bufferSamplers.emissive,  Stage::compute_shader},
            {_rc.inDepth,               _rc.depthSampler,               Stage::compute_shader},
            {_rc.inSkybox,              _rc.skySampler,                 Stage::compute_shader},
            {_rc.cascadeImg,            0,                              Stage::compute_shader},
        };
        kage::pushBindings(pushBinds, COUNTOF(pushBinds));

        std::vector<kage::Binding> setBinds;
        setBinds.emplace_back(kage::Binding{ _rc.brx.sdfAtlas, 0, _rc.brxAtlasSamp, Stage::compute_shader }); // 0
        setBinds.emplace_back(kage::Binding{ _rc.brx.cascadeInfos, BindingAccess::read, Stage::compute_shader }); // 1
        setBinds.emplace_back(kage::Binding{ _rc.brx.brickAABB, BindingAccess::read, Stage::compute_shader }); // 2

        for (uint32_t jj = 0; jj < FFX_BRIXELIZER_MAX_CASCADES; jj++) // 3
            setBinds.emplace_back(kage::Binding{ _rc.brx.cascadeAABBTrees[jj], BindingAccess::read, Stage::compute_shader });
        for (uint32_t jj = 0; jj < FFX_BRIXELIZER_MAX_CASCADES; jj++) // 4
            setBinds.emplace_back(kage::Binding{ _rc.brx.cascadeBrickMaps[jj], BindingAccess::read, Stage::compute_shader });

        uint32_t arrayCounts[] = { 1, 1, 1, FFX_BRIXELIZER_MAX_CASCADES, FFX_BRIXELIZER_MAX_CASCADES};

        kage::bindBindings(setBinds.data(), uint16_t(setBinds.size()), arrayCounts, COUNTOF(arrayCounts));


        uint32_t groupCnt = prob_sideCount * ray_sideCount;
        kage::dispatch(groupCnt, groupCnt, prob_sideCount);
    }

    kage::endRec();
}

struct alignas(16) RCMergeData
{
    uint32_t probe_sideCount;
    uint32_t ray_gridSideCount;
    uint32_t currLv;
    uint32_t startLv;
    uint32_t endLv;
    uint32_t idxType;
    uint32_t offset;

    uint32_t c0_probeSideCount;
    uint32_t c0_raySideCount;
};


void prepareRCMerge(RadianceCascadeMerge& _rc, const RCMergeInit& _init, bool _rayPrime = false)
{
    const char* name = _rayPrime ? "merge_radiance_cascade_ray" : "merge_radiance_cascade";

    // merge cascade image
    kage::ShaderHandle cs = kage::registShader(name, "shader/rc_merge_cascade.comp.spv");
    kage::ProgramHandle program = kage::registProgram(name, { cs }, sizeof(RCMergeData));


    int pipelineSpecs[] = { _rayPrime };
    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc{};
    passDesc.prog = program;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecData = (void*)pConst->data;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);

    kage::PassHandle pass = kage::registPass(name, passDesc);

    uint32_t lvFactor = uint32_t(1 << _init.currCas);
    uint32_t probSideCnt = kage::k_rclv0_probeSideCount / lvFactor;
    uint32_t resolution_xy = _rayPrime 
                                ? kage::k_rclv0_probeSideCount * kage::k_rclv0_rayGridSideCount 
                                : probSideCnt;
    uint32_t resolution_z = _rayPrime ? probSideCnt * 2 : probSideCnt;

    const char* imgName = _rayPrime ? "merged_cascade_ray" : "merged_cascade_probe";
    kage::ImageDesc mergedImgDesc{};
    mergedImgDesc.width = resolution_xy;
    mergedImgDesc.height = resolution_xy;
    mergedImgDesc.numLayers = resolution_z;
    mergedImgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    mergedImgDesc.depth = 1;
    mergedImgDesc.numMips = 1;
    mergedImgDesc.type = kage::ImageType::type_2d;
    mergedImgDesc.viewType = kage::ImageViewType::type_2d_array;
    mergedImgDesc.usage = ImgUsage::transfer_dst | ImgUsage::storage | ImgUsage::sampled;
    mergedImgDesc.layout = kage::ImageLayout::general;
    kage::ImageHandle mergedCas = kage::registTexture(imgName, mergedImgDesc, nullptr, kage::ResourceLifetime::non_transition);

    kage::ImageHandle mergedCasAlias = kage::alias(mergedCas);


    kage::bindImage(pass, mergedCas
        , Stage::compute_shader
        , Access::shader_write | Access::shader_read
        , kage::ImageLayout::general
        , mergedCasAlias
    );
    
    kage::SamplerHandle skySamp = kage::sampleImage(pass, _init.skybox
        , Stage::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::SamplerHandle rcSamp = kage::sampleImage(pass, _init.radianceCascade
        , Stage::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle linearSamp = kage::sampleImage(pass, mergedCas
        , Stage::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle nearedSamp = kage::sampleImage(pass, mergedCas
        , Stage::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::min
    );

    _rc.pass = pass;
    _rc.program = program;
    _rc.cs = cs;

    _rc.skybox = _init.skybox;
    _rc.skySampler = skySamp;
    _rc.radianceCascade = _init.radianceCascade;
    _rc.rcSampler = rcSamp;

    _rc.linearSamp = linearSamp;
    _rc.nearedSamp = nearedSamp;

    _rc.mergedCascade = mergedCas;
    _rc.mergedCascadesAlias = mergedCasAlias;

    _rc.currLv = _init.currCas;
    _rc.rayPrime = _rayPrime;
}

void updateRCMerge(RadianceCascadeMerge& _rc, const Dbg_RadianceCascades& _dbg)
{
    // should update the merged ascade(probe) resolution if currLv changed
    uint32_t currCas = _dbg.startCascade;
    if (!_rc.rayPrime && currCas == _rc.currLv)
        return;

    uint32_t lvFactor = uint32_t(1 << currCas);
    uint32_t probSideCnt = kage::k_rclv0_probeSideCount / lvFactor;
    uint32_t resolution_xy = _rc.rayPrime 
                                ? (kage::k_rclv0_probeSideCount * kage::k_rclv0_rayGridSideCount) 
                                : probSideCnt;
    uint32_t resolution_z = _rc.rayPrime ? (probSideCnt * 2) : probSideCnt;

    kage::updateImage(_rc.mergedCascade, resolution_xy, resolution_xy, resolution_z);

    _rc.currLv = currCas;
}

void recRCMerge(const RadianceCascadeMerge& _rc, const Dbg_RadianceCascades& _dbg)
{
    kage::startRec(_rc.pass);

    uint32_t startLv = _rc.currLv;
    uint32_t cascadeCount = glm::min(kage::k_rclv0_cascadeLv, _dbg.cascadeCount);
    uint32_t endLv = _rc.rayPrime ? (startLv + cascadeCount) : _rc.currLv + 1;

    uint32_t offsets[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint32_t offset = 0u;
    for (int ii = 0; ii < 8; ++ii) {
        offsets[ii] = offset;
        offset += kage::k_rclv0_probeSideCount >> ii;
    }

    // for ray prime: https://mini.gmshaders.com/p/radiance-cascades
    // there would be nearly 4(next lv ray dir) * 8(next lv probes) = 32 samples
    // from [endLv - 1, startLv]
    for (uint32_t ii = endLv; ii > startLv ; --ii)
    {
        uint32_t lv = ii - 1;
        uint32_t lvFactor = uint32_t(1 << lv);
        uint32_t probSideCnt = kage::k_rclv0_probeSideCount / lvFactor;
        uint32_t raySideCnt = kage::k_rclv0_rayGridSideCount * lvFactor;
        uint32_t resolution_xy = _rc.rayPrime ? probSideCnt * raySideCnt : probSideCnt;

        RCMergeData data;
        data.probe_sideCount = probSideCnt;
        data.ray_gridSideCount = raySideCnt;
        data.currLv = lv;
        data.startLv = startLv;
        data.endLv = endLv;
        data.idxType = _dbg.idx_type;
        data.offset = offsets[lv];
        data.c0_probeSideCount = kage::k_rclv0_probeSideCount;
        data.c0_raySideCount = kage::k_rclv0_rayGridSideCount;

        const kage::Memory* mem = kage::alloc(sizeof(RCMergeData));
        memcpy(mem->data, &data, mem->size);

        kage::setConstants(mem);
        
        // should sample the original cascades for the [endLv - 1]. The merged cascade
        // Also, using sampler istead of bind image directly is to take advantage of the hardware bilinear sampling
        bool useOriginalCas = _rc.rayPrime && (lv == endLv - 1);
        kage::ImageHandle accessImg = useOriginalCas ? _rc.radianceCascade : _rc.mergedCascade;
        kage::SamplerHandle accessSamp = 
            _rc.rayPrime 
            ? useOriginalCas 
                ? _rc.rcSampler 
                : _rc.linearSamp 
            : _rc.nearedSamp;

        kage::Binding binds[] = {
            {_rc.skybox,            _rc.skySampler,     Stage::compute_shader},
            {_rc.radianceCascade,   _rc.rcSampler,      Stage::compute_shader},
            {accessImg,             accessSamp,         Stage::compute_shader}, 
            {_rc.mergedCascade,     0,                  Stage::compute_shader}, // current lv
        };

        kage::pushBindings(binds, COUNTOF(binds));

        kage::dispatch(resolution_xy, resolution_xy, probSideCnt);
    }

    kage::endRec();
}



void prepareRadianceCascade(RadianceCascade& _rc, const RadianceCascadeInitData _init)
{
    RCBuildInit rcInit{};
    rcInit.g_buffer = _init.g_buffer;
    rcInit.depth = _init.depth;
    rcInit.skybox = _init.skybox;
    memcpy(&rcInit.brx, &_init.brx, sizeof(BRX_UserResources));
    prepareRCbuild(_rc.build, rcInit);

    {
        RCMergeInit mergeInit{};
        mergeInit.radianceCascade = _rc.build.radCascdOutAlias;
        mergeInit.skybox = _init.skybox;
        mergeInit.currCas = _init.currCas;
        prepareRCMerge(_rc.mergeRay, mergeInit, true);
    }

    {
        RCMergeInit mergeInit{};
        mergeInit.radianceCascade = _rc.mergeRay.mergedCascadesAlias;
        mergeInit.skybox = _init.skybox;
        mergeInit.currCas = _init.currCas;
        prepareRCMerge(_rc.mergeProbe, mergeInit, false);
    }
}

void updateRadianceCascade(
    RadianceCascade& _rc
    , const Dbg_RadianceCascades& _dbgRcBuild
    , const TransformData& _trans
)
{
    updateRCBuild(_rc.build, _dbgRcBuild, _trans);
    recRCBuild(_rc.build, _dbgRcBuild);

    recRCMerge(_rc.mergeRay, _dbgRcBuild);
    recRCMerge(_rc.mergeProbe, _dbgRcBuild);
}
