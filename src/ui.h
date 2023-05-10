#pragma once

struct PushConstBlock {
    glm::vec2 scale;
    glm::vec2 translate;
};

struct UIRendering
{
    VkDevice device;
    VkQueue queue;
    
    Program program;

    VkPipeline pipeline;

    // shaders
    Shader vs;
    Shader fs;

    // vertex buffer and index buffer
    Buffer vb;
    uint32_t vtxCount;
    Buffer ib;
    uint32_t idxCount;

    // image and for descriptor set push
    Image   fontImage;
    VkSampler sampler;

    // constants
    PushConstBlock pushConstBlock;
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

void initializeUIRendering(UIRendering& ui, VkDevice device, VkQueue queue, float scale = 1.f);
void destroyUIRendering(UIRendering& ui);

void prepareUIPipeline(UIRendering& ui, const VkPipelineCache pipelineCache, const VkPipelineRenderingCreateInfo& renderInfo);
void prepareUIResources(UIRendering& ui, const VkPhysicalDeviceMemoryProperties& memoryProps, VkCommandPool cmdPool, bool useChinese = false);

void updateUIRendering(UIRendering& ui, const VkPhysicalDeviceMemoryProperties& memoryProps);
void drawUI(UIRendering& ui, const VkCommandBuffer cmdBuffer);

void updateImGui(const Input& input, RenderOptionsData& rd, const ProfilingData& pd, const LogicData& ld);