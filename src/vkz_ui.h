#pragma once

#include "vkz.h"
#include "uidata.h"
#include "math.h"

struct PushConstBlock {
    glm::vec2 scale;
    glm::vec2 translate;
};

struct UIRendering
{
    vkz::ProgramHandle program{ vkz::kInvalidHandle };

    // shaders
    vkz::ShaderHandle vs{ vkz::kInvalidHandle };
    vkz::ShaderHandle fs{ vkz::kInvalidHandle };

    vkz::PassHandle pass{ vkz::kInvalidHandle };

    // vertex buffer and index buffer
    vkz::BufferHandle vb{ vkz::kInvalidHandle };
    uint32_t vtxCount;
    vkz::BufferHandle ib{ vkz::kInvalidHandle };
    uint32_t idxCount;

    // image and for descriptor set push
    vkz::ImageHandle fontImage{ vkz::kInvalidHandle };
};

struct Input
{
    float mousePosx, mousePosy;

    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } mouseButtons;

    float width, height;
};

void vkz_prepareUI(UIRendering& _ui, float _scale = 1.f, bool _useChinese = false);
void vkz_destroyUIRendering(UIRendering& ui);

void vkz_updateImGui(const Input& input, RenderOptionsData& rd, const ProfilingData& pd, const LogicData& ld);

void vkz_updateUIRenderData(UIRendering& ui);
