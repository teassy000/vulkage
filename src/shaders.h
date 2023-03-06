#pragma once

struct Shader
{
    VkShaderModule module;
    VkShaderStageFlagBits stage;

    VkDescriptorType resourceTypes[32];
    uint32_t resourceMask;

    uint32_t localSizeX;
    uint32_t localSizeY;
    uint32_t localSizeZ;

    bool usesPushConstants;
};

struct Program
{
    VkPipelineLayout        layout;
    VkDescriptorSetLayout   setLayout;
};

using Shaders = std::initializer_list<const Shader*>;

bool loadShader(Shader& shader, VkDevice device, const char* path);

VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, VkRenderPass renderPass, Shaders shaders);

VkDescriptorSetLayout createSetLayout(VkDevice device, Shaders shaders);
VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout outSetLayout);

Program createProgram(VkDevice device, Shaders shaders);
void destroyProgram(VkDevice device, const Program& program);