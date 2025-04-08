#include "vkz_radiance_cascade.h"
#include "config.h"
#include "mesh.h"



using Binding = kage::Binding;
using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;
using LoadOp = kage::AttachmentLoadOp;
using StoreOp = kage::AttachmentStoreOp;

struct RCBuildInit
{
    GBuffer g_buffer;
    kage::ImageHandle depth;
    kage::ImageHandle skybox;

    BRX_UserResources brx;
};

void prepareRCbuild(RadianceCascadeBuild& _rc, const RCBuildInit& _init)
{
    // build the cascade image
    kage::ShaderHandle cs = kage::registShader("build_cascade", "shaders/rc_build_cascade.comp.spv");
    kage::ProgramHandle program = kage::registProgram("build_cascade", { cs }, sizeof(RadianceCascadesConfig));

    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
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
    imgDesc.usage = kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::storage | kage::ImageUsageFlagBits::sampled;
    imgDesc.layout = kage::ImageLayout::general;

    kage::ImageHandle img = kage::registTexture("cascade", imgDesc, nullptr, kage::ResourceLifetime::non_transition);
    kage::ImageHandle outAlias = kage::alias(img);

    // transform buffer
    kage::BufferDesc transDesc{};
    transDesc.size = sizeof(RadianceCascadesTransform);
    transDesc.usage = kage::BufferUsageFlagBits::uniform | kage::BufferUsageFlagBits::transfer_dst;
    kage::BufferHandle trans = kage::registBuffer("rc_trans", transDesc);

    kage::bindBuffer(pass, trans
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::SamplerHandle albedoSamp = kage::sampleImage(pass, _init.g_buffer.albedo
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle normSamp = kage::sampleImage(pass, _init.g_buffer.normal
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle wpSamp = kage::sampleImage(pass, _init.g_buffer.worldPos
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle emmiSamp = kage::sampleImage(pass, _init.g_buffer.emissive
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle depthSamp = kage::sampleImage(pass, _init.depth
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle skySamp = kage::sampleImage(pass, _init.skybox
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::bindImage(pass, img
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , outAlias
    );

    kage::SamplerHandle brxAtlasSamp = kage::sampleImage(pass, _init.brx.sdfAtlas
        , kage::PipelineStageFlagBits::compute_shader
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

void recRCBuild(const RadianceCascadeBuild& _rc , const Dbg_RCBuild& _dbg)
{
    kage::startRec(_rc.pass);

    for (uint32_t ii = 0; ii < kage::k_rclv0_cascadeLv; ++ii)
    {
        uint32_t    level_factor    = uint32_t(1 << ii);
        uint32_t    prob_sideCount  = kage::k_rclv0_probeSideCount / level_factor;
        uint32_t    prob_count      = uint32_t(pow(prob_sideCount, 3));
        uint32_t    ray_sideCount   = kage::k_rclv0_rayGridSideCount * level_factor;
        uint32_t    ray_count       = ray_sideCount * ray_sideCount; // each probe has a 2d grid of rays
        float prob_sideLen = _dbg.totalRadius * 2.f / float(prob_sideCount);
        float rayLen = length(vec3(prob_sideLen)) * .5f;

        RadianceCascadesConfig config;
        config.probe_sideCount = prob_sideCount;
        config.ray_gridSideCount = ray_sideCount;
        config.level = ii;
        config.layerOffset = (kage::k_rclv0_probeSideCount - prob_sideCount) * 2;
        config.rayLength = rayLen;
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

        config.debug_type = _dbg.debug_type;

        const kage::Memory* mem = kage::alloc(sizeof(RadianceCascadesConfig));
        memcpy(mem->data, &config, mem->size);

        kage::setConstants(mem);

        kage::Binding pushBinds[] =
        {
            {_rc.trans,                 Access::read,                   Stage::compute_shader},
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
        setBinds.emplace_back(kage::Binding{ _rc.brx.cascadeInfos, Access::read, Stage::compute_shader }); // 1
        setBinds.emplace_back(kage::Binding{ _rc.brx.brickAABB, Access::read, Stage::compute_shader }); // 2

        for (uint32_t jj = 0; jj < FFX_BRIXELIZER_MAX_CASCADES; jj++) // 3
            setBinds.emplace_back(kage::Binding{ _rc.brx.cascadeAABBTrees[jj], Access::read, Stage::compute_shader });
        for (uint32_t jj = 0; jj < FFX_BRIXELIZER_MAX_CASCADES; jj++) // 4
            setBinds.emplace_back(kage::Binding{ _rc.brx.cascadeBrickMaps[jj], Access::read, Stage::compute_shader });

        uint32_t arrayCounts[] = { 1, 1, 1, FFX_BRIXELIZER_MAX_CASCADES, FFX_BRIXELIZER_MAX_CASCADES};

        kage::bindBindings(setBinds.data(), uint16_t(setBinds.size()), arrayCounts, COUNTOF(arrayCounts));


        uint32_t groupCnt = prob_sideCount * ray_sideCount;
        kage::dispatch(groupCnt, groupCnt, prob_sideCount);
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
    prepareRCbuild(_rc.rcBuild, rcInit);
}

void updateRadianceCascade(
    RadianceCascade& _rc
    , const Dbg_RCBuild& _dbgRcBuild
    , const TransformData& _trans
)
{
    // update trans buf
    RadianceCascadesTransform trans;

    if (_dbgRcBuild.followCam) {
        _rc.rcBuild.cameraPos = _trans.cameraPos;
    }
    

    trans.cameraPos = _rc.rcBuild.cameraPos;
    trans.view = _trans.view;
    trans.proj = _trans.proj;

    const kage::Memory* memTransform = kage::alloc(sizeof(RadianceCascadesTransform));
    memcpy_s(memTransform->data, memTransform->size, &trans, sizeof(RadianceCascadesTransform));
    kage::updateBuffer(_rc.rcBuild.trans, memTransform);



    recRCBuild(_rc.rcBuild, _dbgRcBuild);
}
