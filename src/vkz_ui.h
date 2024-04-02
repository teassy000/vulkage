#pragma once

#include "vkz.h"
#include "uidata.h"
#include "vkz_math.h"

struct PushConstBlock {
    glm::vec2 scale;
    glm::vec2 translate;
};

struct UIRendering
{
    kage::ProgramHandle program{ kage::kInvalidHandle };

    // shaders
    kage::ShaderHandle vs{ kage::kInvalidHandle };
    kage::ShaderHandle fs{ kage::kInvalidHandle };

    kage::PassHandle pass{ kage::kInvalidHandle };

    // vertex buffer and index buffer
    kage::BufferHandle vb{ kage::kInvalidHandle };
    uint32_t vtxCount;
    kage::BufferHandle ib{ kage::kInvalidHandle };
    uint32_t idxCount;

    // image and for descriptor set push
    kage::ImageHandle fontImage{ kage::kInvalidHandle };

    kage::ImageHandle color{ kage::kInvalidHandle };
    kage::ImageHandle depth{ kage::kInvalidHandle };
    
    kage::ImageHandle colorOutAlias{ kage::kInvalidHandle };
    kage::ImageHandle depthOutAlias{ kage::kInvalidHandle };
};


void vkz_prepareUI(UIRendering& _ui, kage::ImageHandle _color, kage::ImageHandle _depth, float _scale = 1.f, bool _useChinese = false);
void vkz_destroyUIRendering(UIRendering& ui);

void vkz_updateImGui(const UIInput& input, DebugRenderOptionsData& rd, const DebugProfilingData& pd, const DebugLogicData& ld);

void vkz_updateUIRenderData(UIRendering& ui);
