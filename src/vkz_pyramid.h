#pragma once

#include "kage.h"
#include "config.h"


struct PyramidRendering {
    kage::PassHandle pass;
    kage::ProgramHandle program;
    kage::ShaderHandle cs;

    kage::ImageHandle inDepth{ kage::kInvalidHandle };
    kage::ImageHandle image{ kage::kInvalidHandle };
    kage::ImageHandle imgOutAlias{ kage::kInvalidHandle };
    kage::SamplerHandle sampler{ kage::kInvalidHandle };

    uint32_t width;
    uint32_t height;
    uint32_t levels;
};

void preparePyramid(
    PyramidRendering& _pyramid
    , uint32_t _width
    , uint32_t _height
);

void setPyramidPassDependency(PyramidRendering& _pyramid, const kage::ImageHandle _inDepth);

void updatePyramid(PyramidRendering& _pyramid, uint32_t _rtWidth, uint32_t _rtHeight);