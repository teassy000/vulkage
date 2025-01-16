#pragma once

#include "kage.h"

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

    kage::ImageHandle outColor;
    kage::ImageHandle outColorAlias;
};

const GBuffer createGBuffer();
const GBuffer aliasGBuffer(const GBuffer& _gb);
void initDeferredShading(DeferredShading& _ds, const GBuffer& _gb, const kage::ImageHandle _rt);
void updateDeferredShading(const DeferredShading& _ds, const uint32_t _w, const uint32_t _h);