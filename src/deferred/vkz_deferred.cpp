#include "deferred/vkz_deferred.h"
#include "kage_math.h"

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

    return gb;
}

const GBuffer aliasGBuffer(const GBuffer& _gb)
{
    GBuffer result;
    result.albedo = kage::alias(_gb.albedo);
    result.normal = kage::alias(_gb.normal);
    result.worldPos = kage::alias(_gb.worldPos);
    result.emissive = kage::alias(_gb.emissive);

    return result;
}

void initDeferredShading(DeferredShading& _ds, const GBuffer& _gb, const kage::ImageHandle _sky)
{
    kage::ShaderHandle cs = kage::registShader("deferred", "shaders/deferred.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("deferred", { cs }, sizeof(glm::vec2));

    kage::PassDesc desc;
    desc.programId = prog.id;
    desc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("deferred", desc);


    kage::ImageDesc outColorDesc;
    outColorDesc.depth = 1;
    outColorDesc.numLayers = 1;
    outColorDesc.numMips = 1;
    outColorDesc.usage = kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage;
    kage::ImageHandle outColor = kage::registRenderTarget("deferred_out_color", outColorDesc, kage::ResourceLifetime::non_transition);

    kage::SamplerHandle albedoSamp = kage::sampleImage(pass, _gb.albedo
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle normSamp = kage::sampleImage(pass, _gb.normal
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle wpSamp = kage::sampleImage(pass, _gb.worldPos
        , 2
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle emmiSamp = kage::sampleImage(pass, _gb.emissive
        , 3
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle skySamp = kage::sampleImage(pass, _sky
        , 3
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    _ds.outColorAlias = kage::alias(outColor);
    kage::bindImage(pass, outColor
        , 5
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , _ds.outColorAlias
    );


    _ds.pass = pass;
    _ds.prog = prog;
    _ds.cs = cs;
    _ds.outColor = outColor;
    _ds.gBuffer = _gb;
    

    _ds.gBufSamplers.albedo = albedoSamp;
    _ds.gBufSamplers.normal = normSamp;
    _ds.gBufSamplers.worldPos = wpSamp;
    _ds.gBufSamplers.emissive = emmiSamp;

    _ds.inSky = _sky;
    _ds.skySampler = skySamp;
}

void recDeferredShading(const DeferredShading& _ds, const uint32_t _w, const uint32_t _h)
{
    kage::startRec(_ds.pass);
    vec2 resolution = vec2(_w, _h);
    const kage::Memory* mem = kage::alloc(sizeof(resolution));
    memcpy(mem->data, &resolution, sizeof(resolution));
    kage::setConstants(mem);

    using Stage = kage::PipelineStageFlagBits::Enum;
    using Access = kage::BindingAccess;
    kage::Binding binds[] =
    {
        {_ds.gBuffer.albedo,    _ds.gBufSamplers.albedo,    Stage::compute_shader},
        {_ds.gBuffer.normal,    _ds.gBufSamplers.normal,    Stage::compute_shader},
        {_ds.gBuffer.worldPos,  _ds.gBufSamplers.worldPos,  Stage::compute_shader},
        {_ds.gBuffer.emissive,  _ds.gBufSamplers.emissive,  Stage::compute_shader},
        {_ds.inSky,             _ds.skySampler,             Stage::compute_shader},
        {_ds.outColor,          0,                          Stage::compute_shader},
    };

    kage::pushBindings(binds, COUNTOF(binds));

    kage::dispatch(_w, _h, 1);

    kage::endRec();
}

void updateDeferredShading(const DeferredShading& _ds, const uint32_t _w, const uint32_t _h)
{
    recDeferredShading(_ds, _w, _h);
}
