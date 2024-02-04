#pragma once

#include "vkz.h"

struct SpecImageViewConfig
{
    vkz::ImageHandle image;
    uint32_t baseMipLevel;
    uint32_t levels;
};

struct PyramidRendering {
    vkz::ImageHandle pyramid{ vkz::kInvalidHandle };
    vkz::ProgramHandle program;
    vkz::ShaderHandle cs;
    vkz::PassHandle pass;
    
    uint32_t width;
    uint32_t height;

    uint32_t levels;
    uint32_t level_width;

    SpecImageViewConfig* specImageViewConfigs;
    uint32_t specImageViewConfigCount;
};

void preparePyramid(
    PyramidRendering& _pyramid
    , uint32_t _width
    , uint32_t _height
    , uint32_t _levels
    , uint32_t _level_width
    , SpecImageViewConfig* _specImageViewConfigs
    , uint32_t _specImageViewConfigCount
);
