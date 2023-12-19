#pragma once

namespace vkz
{
    struct Shader_vk
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

    struct Program_vk
    {
        VkPipelineLayout        layout;
        VkDescriptorSetLayout   setLayout;

        VkDescriptorUpdateTemplate updateTemplate;

        VkShaderStageFlags pushConstantStages;
    };

    struct PipelineConfigs_vk
    {
        bool enableDepthTest{ true };
        bool enableDepthWrite{ true };
        VkCompareOp depthCompOp{ VK_COMPARE_OP_GREATER };
    };
    using Constants = std::initializer_list<int>;

    bool loadShader(Shader_vk& shader, VkDevice device, const char* path);

    VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const VkPipelineRenderingCreateInfo& renderInfo
        , const std::vector<Shader_vk>& shaders, VkPipelineVertexInputStateCreateInfo* vtxInputState, const std::vector<int> constants = {}, const PipelineConfigs_vk& pipeConfigs = {});
    VkPipeline createComputePipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const Shader_vk& shader, const std::vector<int> constants = {});

    VkDescriptorSetLayout createSetLayout(VkDevice device, const std::vector<Shader_vk>& shaders);
    VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout outSetLayout, VkShaderStageFlags pushConstantStages, size_t pushConstantSize);

    Program_vk createProgram(VkDevice device, VkPipelineBindPoint bindingPoint, const std::vector<Shader_vk>& shaders, size_t pushConstantSize = 0);
    void destroyProgram(VkDevice device, const Program_vk& program);

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

}