#pragma once

#include "kage.h"
#include "kage_math.h"
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

struct RCAccessData
{
    uint32_t lv;
    uint32_t raySideCount;
    uint32_t probeSideCount;
    uint32_t layerOffset;
    float rayLen;
    float probeSideLen;
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
    kage::ImageHandle radianceCascade;
    kage::SamplerHandle rcSampler;

    kage::ImageHandle outColor;
    kage::ImageHandle outColorAlias;
};

const GBuffer createGBuffer();
const GBuffer aliasGBuffer(const GBuffer& _gb);
void initDeferredShading(DeferredShading& _ds, const GBuffer& _gb, const kage::ImageHandle _rt, const kage::ImageHandle _rc);
void updateDeferredShading(const DeferredShading& _ds, const uint32_t _w, const uint32_t _h, const vec3 _camPos, const float _tatalRadius, const uint32_t _idxType, const Dbg_RadianceCascades& _rc);