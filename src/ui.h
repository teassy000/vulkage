#pragma once

struct PushConstBlock {
    glm::vec2 scale;
    glm::vec2 translate;
};

struct UI
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

void initializeUI(UI& ui, VkDevice device, VkQueue queue);
void destroyUI(UI& ui);

void prepareUIPipeline(UI& ui, const VkPipelineCache pipelineCache, const VkRenderPass renderPass);
void prepareUIResources(UI& ui, const VkPhysicalDeviceMemoryProperties& memoryProps, VkCommandPool cmdPool);

void updateUI(UI& ui, const VkPhysicalDeviceMemoryProperties& memoryProps);
void drawUI(UI& ui, const VkCommandBuffer cmdBuffer);

