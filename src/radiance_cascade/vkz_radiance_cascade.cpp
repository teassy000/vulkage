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
    kage::ImageHandle sdfAtlas;
    kage::ImageHandle depth;
    kage::ImageHandle skybox;
    kage::BufferHandle trans;
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

    kage::bindBuffer(pass, _init.trans
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

    kage::bindImage(pass, _init.sdfAtlas
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
        , kage::ImageLayout::general
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

    _rc.trans = _init.trans;
    _rc.inSdfAtlas = _init.sdfAtlas;
    _rc.inDepth = _init.depth;
    _rc.depthSampler = depthSamp;

    _rc.cascadeImg = img;
    _rc.radCascdOutAlias = outAlias;
}

void recRCBuild(const RadianceCascadeBuild& _rc, const float _sceneRadius)
{
    kage::startRec(_rc.pass);

    for (uint32_t ii = 0; ii < kage::k_rclv0_cascadeLv; ++ii)
    {
        uint32_t    level_factor    = uint32_t(1 << ii);
        uint32_t    prob_sideCount  = kage::k_rclv0_probeSideCount / level_factor;
        uint32_t    prob_count      = uint32_t(pow(prob_sideCount, 3));
        uint32_t    ray_sideCount   = kage::k_rclv0_rayGridSideCount * level_factor;
        uint32_t    ray_count       = ray_sideCount * ray_sideCount; // each probe has a 2d grid of rays
        float prob_sideLen = _sceneRadius * 2.f / float(prob_sideCount);
        float rayLen = length(vec3(prob_sideLen)) * .5f;

        RadianceCascadesConfig config;
        config.probe_sideCount = prob_sideCount;
        config.ray_gridSideCount = ray_sideCount;
        config.level = ii;
        config.layerOffset = (kage::k_rclv0_probeSideCount - prob_sideCount) * 2;
        config.rayLength = rayLen;
        config.probeSideLen = prob_sideLen;

        const kage::Memory* mem = kage::alloc(sizeof(RadianceCascadesConfig));
        memcpy(mem->data, &config, mem->size);

        kage::setConstants(mem);

        kage::Binding binds[] =
        {
            {_rc.trans,                 Access::read,                   Stage::compute_shader},
            {_rc.g_buffer.albedo,       _rc.g_bufferSamplers.albedo,    Stage::compute_shader},
            {_rc.g_buffer.normal,       _rc.g_bufferSamplers.normal,    Stage::compute_shader},
            {_rc.g_buffer.worldPos,     _rc.g_bufferSamplers.worldPos,  Stage::compute_shader},
            {_rc.g_buffer.emissive,     _rc.g_bufferSamplers.emissive,  Stage::compute_shader},
            {_rc.inDepth,               _rc.depthSampler,               Stage::compute_shader},
            {_rc.inSkybox,              _rc.skySampler,                 Stage::compute_shader},
            {_rc.cascadeImg,            0,                              Stage::compute_shader},
            {_rc.inSdfAtlas,            0,                              Stage::compute_shader},
        };
        kage::pushBindings(binds, COUNTOF(binds));

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
    rcInit.sdfAtlas = _init.sdfAtlas;
    rcInit.skybox = _init.skybox;
    rcInit.trans = _init.transBuf;
    prepareRCbuild(_rc.rcBuild, rcInit);
}

void updateRadianceCascade(
    const RadianceCascade& _rc
    , uint32_t _drawCount
    , const DrawCull& _camCull
    , const uint32_t _width
    , const uint32_t _height
    , const float _sceneRadius
)
{
    recRCBuild(_rc.rcBuild, _sceneRadius);
}
