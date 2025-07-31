#pragma once

#include "core/kage.h"
#include "core/kage_math.h"
#include "demo_structs.h"

struct GBuffer
{
    kage::ImageHandle albedo;
    kage::ImageHandle normal;
    kage::ImageHandle worldPos;
    kage::ImageHandle emissive;
    kage::ImageHandle specular;
};

struct GBufferSamplers
{
    kage::SamplerHandle albedo;
    kage::SamplerHandle normal;
    kage::SamplerHandle worldPos;
    kage::SamplerHandle emissive;
    kage::SamplerHandle specular;
};

struct RadianceCascadesData
{
    kage::ImageHandle cascades{};
    kage::ImageHandle mergedCascade{};
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

    kage::BufferHandle rcAccessData;
    kage::ImageHandle radianceCascades;
    kage::SamplerHandle rcSampler;

    kage::ImageHandle rcMergedData;
    kage::SamplerHandle rcMergedSampler;

    kage::ImageHandle outColor;
    kage::ImageHandle outColorAlias;
};

const GBuffer createGBuffer();
const GBuffer aliasGBuffer(const GBuffer& _gb);
void initDeferredShading(DeferredShading& _ds, const GBuffer& _gb, const kage::ImageHandle _sky, const RadianceCascadesData& _rcData);
void updateDeferredShading(const DeferredShading& _ds, const uint32_t _w, const uint32_t _h, const vec3 _camPos, const float _tatalRadius, const uint32_t _idxType, const Dbg_RadianceCascades& _rc);