#include "common.h"
#include "kage.h"

struct AntiAliasingPass
{
    kage::PassHandle pass{ kage::kInvalidHandle };
    kage::ProgramHandle program{ kage::kInvalidHandle };
    kage::ShaderHandle cs{ kage::kInvalidHandle };

    kage::ImageHandle color{ kage::kInvalidHandle };
    kage::ImageHandle colorOutAlias{ kage::kInvalidHandle };
    kage::SamplerHandle sampler{ kage::kInvalidHandle };

    uint32_t w{ 0 };
    uint32_t h{ 0 };
};

void prepareAA(AntiAliasingPass& _aa, uint32_t _width, uint32_t _height);
void setAAPassDependency(AntiAliasingPass& _aa, const kage::ImageHandle _inColor);
void updateAA(AntiAliasingPass& _aa, uint32_t _rtWidth, uint32_t _rtHeight);