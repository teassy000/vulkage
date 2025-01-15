#pragma once

#include "kage_math.h"
#include "common.h"
#include "kage.h"

struct alignas(16) RadianceCascadesConfig
{
    uint32_t rayGridDiameter;
    uint32_t probeDiameter;
    uint32_t level;
    uint32_t layerOffset;
};

struct RadianceCascade
{
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle cs;

    kage::ImageHandle cascadeImg;
    kage::ImageHandle outAlias;

    RadianceCascadesConfig lv0Config;
};

void prepareRadianceCascade(RadianceCascade& _rc, uint32_t _w, uint32_t _h);
void updateRadianceCascade(const RadianceCascade& _rc);