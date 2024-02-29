#pragma once

#include "vkz.h"
#include "config.h"


struct PyramidRendering {
    vkz::ImageHandle inDepth{ vkz::kInvalidHandle };

    vkz::ImageHandle image{ vkz::kInvalidHandle };
    vkz::ImageHandle imgOutAlias{ vkz::kInvalidHandle };
    vkz::SamplerHandle sampler{ vkz::kInvalidHandle };

    vkz::ProgramHandle program;
    vkz::ShaderHandle cs;
    vkz::PassHandle pass;
    
    uint32_t width;
    uint32_t height;
    uint32_t levels;
    
    vkz::ImageViewHandle imgMips[vkz::kMaxNumOfImageMipLevel]{ vkz::kInvalidHandle }; // [levels]
};

void preparePyramid(
    PyramidRendering& _pyramid
    , uint32_t _width
    , uint32_t _height
);

void setPyramidPassDependency(PyramidRendering& _pyramid, const vkz::ImageHandle _inDepth);
