#pragma once

#include "kage.h"

struct GBuffer
{
    kage::ImageHandle albedo;
    kage::ImageHandle normal;
    kage::ImageHandle worldPos;
    kage::ImageHandle emissive;
};

struct DeferredShading
{
    GBuffer gbuffer;

    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;

    kage::ImageHandle outColor;
    kage::ImageHandle outColorAlias;
};

const GBuffer createGBuffer();
const GBuffer aliasGBuffer(const GBuffer& _gb);
bool initDeferredShading(DeferredShading& _ds, const GBuffer& _gb);
bool updateDeferredShading(const DeferredShading& _ds);