#pragma once

#include "kage.h"
#include "uidata.h"
#include "demo_structs.h"


struct UIRendering
{
    kage::PassHandle pass{ kage::kInvalidHandle };
    kage::ProgramHandle program{ kage::kInvalidHandle };

    // shaders
    kage::ShaderHandle vs{ kage::kInvalidHandle };
    kage::ShaderHandle fs{ kage::kInvalidHandle };

    // vertex buffer and index buffer
    kage::BufferHandle vb{ kage::kInvalidHandle };
    uint32_t vtxCount;
    kage::BufferHandle ib{ kage::kInvalidHandle };
    uint32_t idxCount;

    // image and for descriptor set push
    kage::ImageHandle fontImage{ kage::kInvalidHandle };
    kage::SamplerHandle sampler{ kage::kInvalidHandle };

    kage::ImageHandle color{ kage::kInvalidHandle };
    kage::ImageHandle depth{ kage::kInvalidHandle };
    
    kage::ImageHandle colorOutAlias{ kage::kInvalidHandle };
    kage::ImageHandle depthOutAlias{ kage::kInvalidHandle };

    kage::ImageHandle dummyColor{ kage::kInvalidHandle };
    kage::SamplerHandle dummySampler{ kage::kInvalidHandle };
};


void prepareUI(
    UIRendering& _ui
    , kage::ImageHandle _color
    , kage::ImageHandle _depth
    , float _scale = 1.f
    , bool _useChinese = false
);

void destroyUI(UIRendering& _ui);

void updateUI(
    UIRendering& _ui
    , const UIInput& _input
    , DebugFeatures& _features
    , const DebugProfilingData& _pd
    , const DebugLogicData& _ld
);
