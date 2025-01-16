#include "deferred/vkz_deferred.h"

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

bool initDeferredShading(DeferredShading& _ds, const GBuffer& _gb)
{

    return false;
}

bool updateDeferredShading(const DeferredShading& _ds)
{
    return false;
}
