#include "deferred/vkz_deferred.h"
#include "kage_math.h"
#include <vector >

const GBuffer createGBuffer()
{
    GBuffer gb;

    kage::ImageDesc albedoDesc;
    albedoDesc.depth = 1;
    albedoDesc.numLayers = 1;
    albedoDesc.numMips = 1;
    albedoDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage;

    gb.albedo = kage::registRenderTarget("gbuf_albedo", albedoDesc, kage::ResourceLifetime::non_transition);

    kage::ImageDesc normalDesc;
    normalDesc.depth = 1;
    normalDesc.numLayers = 1;
    normalDesc.numMips = 1;
    normalDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage;
    gb.normal = kage::registRenderTarget("gbuf_normal", normalDesc, kage::ResourceLifetime::non_transition);

    kage::ImageDesc worldPosDesc;
    worldPosDesc.depth = 1;
    worldPosDesc.numLayers = 1;
    worldPosDesc.numMips = 1;
    worldPosDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage;
    gb.worldPos = kage::registRenderTarget("gbuf_world_pos", worldPosDesc, kage::ResourceLifetime::non_transition);

    kage::ImageDesc emissiveDesc;
    emissiveDesc.depth = 1;
    emissiveDesc.numLayers = 1;
    emissiveDesc.numMips = 1;
    emissiveDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage;
    gb.emissive = kage::registRenderTarget("gbuf_emissive", emissiveDesc, kage::ResourceLifetime::non_transition);

    kage::ImageDesc specularDesc;
    specularDesc.depth = 1;
    specularDesc.numLayers = 1;
    specularDesc.numMips = 1;
    specularDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage;
    gb.specular = kage::registRenderTarget("specularDesc", specularDesc, kage::ResourceLifetime::non_transition);

    return gb;
}

const GBuffer aliasGBuffer(const GBuffer& _gb)
{
    GBuffer result;
    result.albedo = kage::alias(_gb.albedo);
    result.normal = kage::alias(_gb.normal);
    result.worldPos = kage::alias(_gb.worldPos);
    result.emissive = kage::alias(_gb.emissive);
    result.specular = kage::alias(_gb.specular);

    return result;
}

struct alignas(16) DeferredConstants
{
    float totalRadius;
    uint32_t cascade_0_probGridCount;
    uint32_t cascade_0_rayGridCount;
    uint32_t debugIdxType;

    uint32_t startCascade;
    uint32_t endCascade;

    float w, h;
    float camx, camy, camz;
};

void initDeferredShading(DeferredShading& _ds, const GBuffer& _gb, const kage::ImageHandle _sky, const kage::ImageHandle _radCasc)
{
    kage::ShaderHandle cs = kage::registShader("deferred", "shaders/deferred.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("deferred", { cs }, sizeof(DeferredConstants));

    kage::PassDesc desc;
    desc.programId = prog.id;
    desc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("deferred", desc);

    kage::ImageDesc outColorDesc;
    outColorDesc.depth = 1;
    outColorDesc.numLayers = 1;
    outColorDesc.numMips = 1;
    outColorDesc.usage = kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage;
    kage::ImageHandle outColor = kage::registRenderTarget("deferred_out_color", outColorDesc, kage::ResourceLifetime::non_transition);

    kage::BufferDesc bufDesc;
    bufDesc.size = sizeof(RCAccessData) * kage::k_rclv0_cascadeLv;
    bufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::uniform | kage::BufferUsageFlagBits::transfer_dst;
    kage::BufferHandle rcAccessConfigBuf = kage::registBuffer("defer_rc_configs", bufDesc);

    kage::SamplerHandle albedoSamp = kage::sampleImage(pass, _gb.albedo
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle normSamp = kage::sampleImage(pass, _gb.normal
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle wpSamp = kage::sampleImage(pass, _gb.worldPos
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle emmiSamp = kage::sampleImage(pass, _gb.emissive
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle specSamp = kage::sampleImage(pass, _gb.specular
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle skySamp = kage::sampleImage(pass, _sky
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle rcSamp = kage::sampleImage(pass, _radCasc
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::weighted_average
    );

    _ds.outColorAlias = kage::alias(outColor);
    kage::bindImage(pass, outColor
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , _ds.outColorAlias
    );

    kage::bindBuffer(pass, rcAccessConfigBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    _ds.pass = pass;
    _ds.prog = prog;
    _ds.cs = cs;
    _ds.outColor = outColor;
    _ds.gBuffer = _gb;

    _ds.rcAccessData = rcAccessConfigBuf;
    _ds.radianceCascade = _radCasc;
    _ds.rcSampler = rcSamp;

    _ds.gBufSamplers.albedo = albedoSamp;
    _ds.gBufSamplers.normal = normSamp;
    _ds.gBufSamplers.worldPos = wpSamp;
    _ds.gBufSamplers.emissive = emmiSamp;
    _ds.gBufSamplers.specular = specSamp;

    _ds.inSky = _sky;
    _ds.skySampler = skySamp;
}

void recDeferredShading(const DeferredShading& _ds, const uint32_t _w, const uint32_t _h, const vec3 _camPos, const float _totalRadius, const uint32_t _idxType)
{
    kage::startRec(_ds.pass);
    DeferredConstants consts;
    
    consts.totalRadius = _totalRadius;
    consts.cascade_0_probGridCount = kage::k_rclv0_probeSideCount;
    consts.cascade_0_rayGridCount = kage::k_rclv0_rayGridSideCount;
    consts.startCascade = 0;
    consts.endCascade = kage::k_rclv0_cascadeLv - 1;
    consts.debugIdxType = _idxType;
    consts.w = (float)_w;
    consts.h = (float)_h;
    consts.camx = _camPos.x;
    consts.camy = _camPos.y;
    consts.camz = _camPos.z;

    const kage::Memory* mem = kage::alloc(sizeof(consts));
    memcpy(mem->data, &consts, sizeof(consts));
    kage::setConstants(mem);

    using Stage = kage::PipelineStageFlagBits::Enum;
    using Access = kage::BindingAccess;
    kage::Binding binds[] =
    {
        {_ds.rcAccessData, Access::read,          Stage::compute_shader},
        {_ds.gBuffer.albedo,    _ds.gBufSamplers.albedo,    Stage::compute_shader},
        {_ds.gBuffer.normal,    _ds.gBufSamplers.normal,    Stage::compute_shader},
        {_ds.gBuffer.worldPos,  _ds.gBufSamplers.worldPos,  Stage::compute_shader},
        {_ds.gBuffer.emissive,  _ds.gBufSamplers.emissive,  Stage::compute_shader},
        {_ds.gBuffer.specular,  _ds.gBufSamplers.specular,  Stage::compute_shader},
        {_ds.inSky,             _ds.skySampler,             Stage::compute_shader},
        {_ds.radianceCascade,   _ds.rcSampler,              Stage::compute_shader},
        {_ds.outColor,          0,                          Stage::compute_shader},
    };

    kage::pushBindings(binds, COUNTOF(binds));

    kage::dispatch(_w, _h, 1);

    kage::endRec();
}

void updateDeferredShading(const DeferredShading& _ds, const uint32_t _w, const uint32_t _h, const vec3 _camPos, const float _tatalRadius, const uint32_t _idxType)
{
    std::vector<RCAccessData> consts(kage::k_rclv0_cascadeLv);
    uint32_t offset = 0;
    for (size_t ii = 0; ii < kage::k_rclv0_cascadeLv; ii++)
    {
        uint32_t level_factor = uint32_t(1u << ii);
        uint32_t probeSideCount = kage::k_rclv0_probeSideCount / level_factor;
        uint32_t raySideCount = kage::k_rclv0_rayGridSideCount * level_factor;

        consts[ii].lv = (uint32_t)ii;
        consts[ii].raySideCount = raySideCount;
        consts[ii].probeSideCount = probeSideCount;
        consts[ii].layerOffset = offset;
        consts[ii].rayLen = glm::length(vec3(_tatalRadius)) / float(probeSideCount);
        consts[ii].probeSideLen = _tatalRadius * 2.f / float(probeSideCount);
    }
    
    const kage::Memory* mem = kage::alloc(uint32_t(consts.size() * sizeof(RCAccessData)));
    memcpy(mem->data, consts.data(), mem->size);
    kage::updateBuffer(_ds.rcAccessData, mem);

    recDeferredShading(_ds, _w, _h, _camPos, _tatalRadius, _idxType);
}
