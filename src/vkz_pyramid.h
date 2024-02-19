#pragma once

#include "vkz.h"

struct SpecImageViewConfig
{
    vkz::ImageHandle image;
    uint32_t baseMipLevel;
    uint32_t levels;
};

struct PyramidRendering {
    vkz::ImageHandle inDepth{ vkz::kInvalidHandle };

    vkz::ImageHandle image{ vkz::kInvalidHandle };
    vkz::ImageHandle outAlias{ vkz::kInvalidHandle };
    vkz::SamplerHandle sampler{ vkz::kInvalidHandle };

    vkz::ProgramHandle program;
    vkz::ShaderHandle cs;
    vkz::PassHandle pass;
    
    uint32_t width;
    uint32_t height;
    uint32_t levels;

    SpecImageViewConfig* specImageViewConfigs;
    uint32_t specImageViewConfigCount;
};

void preparePyramid(
    PyramidRendering& _pyramid
    , uint32_t _width
    , uint32_t _height
    , uint32_t _levels
    , SpecImageViewConfig* _specImageViewConfigs
    , uint32_t _specImageViewConfigCount
);

void setPyramidDependency(
    PyramidRendering& _pyramid
    , const vkz::ImageHandle _inDepth
);
