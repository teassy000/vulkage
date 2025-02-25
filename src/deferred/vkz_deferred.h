#pragma once

#include "kage.h"
#include "kage_math.h"

struct GBuffer
{
    kage::ImageHandle albedo;
    kage::ImageHandle normal;
    kage::ImageHandle worldPos;
    kage::ImageHandle emissive;
};

struct GBufferSamplers
{
    kage::SamplerHandle albedo;
    kage::SamplerHandle normal;
    kage::SamplerHandle worldPos;
    kage::SamplerHandle emissive;
};

struct DeferredShading
{
    GBuffer gBuffer;
    GBufferSamplers gBufSamplers;

    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;

    kage::ImageHandle inSky;
    kage::SamplerHandle skySampler;

    kage::ImageHandle radianceCascade;

    kage::ImageHandle outColor;
    kage::ImageHandle outColorAlias;
};

struct alignas(16) DeferredShadingConsts
{
    float sceneRadius;
    uint32_t cascade_lv;
    uint32_t cascade_0_probGridCount;
    uint32_t cascade_0_rayGridCount;
    vec3 camPos;
    vec2 imageSize;
};

const GBuffer createGBuffer();
const GBuffer aliasGBuffer(const GBuffer& _gb);
void initDeferredShading(DeferredShading& _ds, const GBuffer& _gb, const kage::ImageHandle _rt, const kage::ImageHandle _rc);
void updateDeferredShading(const DeferredShading& _ds, const uint32_t _w, const uint32_t _h, const vec3 _camPos, const float _sceneRadius);