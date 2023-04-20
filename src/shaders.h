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

    VkDescriptorUpdateTemplate updateTemplate;

    VkShaderStageFlags pushConstantStages;
};

using Shaders = std::initializer_list<const Shader*>;

bool loadShader(Shader& shader, VkDevice device, const char* path);

VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const VkPipelineRenderingCreateInfo& renderInfo, Shaders shaders, VkPipelineVertexInputStateCreateInfo* vtxInputState, bool isUI = false);
VkPipeline createComputePipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const Shader& shader);

VkDescriptorSetLayout createSetLayout(VkDevice device, Shaders shaders);
VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout outSetLayout, VkShaderStageFlags pushConstantStages, size_t pushConstantSize);

Program createProgram(VkDevice device, VkPipelineBindPoint bindingPoint, Shaders shaders, size_t pushConstantSize = 0);
void destroyProgram(VkDevice device, const Program& program);

inline uint32_t calcGroupCount(uint32_t threadCount, uint32_t localSize)
{
    return (threadCount + localSize - 1) / localSize;
}

struct DescriptorInfo
{
    union
    {
        VkDescriptorBufferInfo buffer;
        VkDescriptorImageInfo image;
    };

    DescriptorInfo()
    {

    }

    DescriptorInfo(VkBuffer _buffer, VkDeviceSize offset, VkDeviceSize range)
    {
        buffer.buffer = _buffer;
        buffer.offset = offset;
        buffer.range = range;
    }

    DescriptorInfo(VkBuffer _buffer)
    {
        buffer.buffer = _buffer;
        buffer.offset = 0;
        buffer.range = VK_WHOLE_SIZE;
    }

    DescriptorInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
    {
        image.sampler = sampler;
        image.imageView = imageView;
        image.imageLayout = imageLayout;
    }

    DescriptorInfo(VkImageView imageView, VkImageLayout imageLayout)
    {
        image.sampler = VK_NULL_HANDLE;
        image.imageView = imageView;
        image.imageLayout = imageLayout;
    }
};