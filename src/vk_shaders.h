#pragma once
#include "common.h"
#include "kage_rhi_vk.h"

namespace kage { namespace vk
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
        bool usesBindless;
    };

    struct Program_vk
    {
        VkPipelineLayout        layout;
        VkDescriptorSetLayout   setLayout;
        VkDescriptorSetLayout   arrayLayout;

        VkDescriptorUpdateTemplate updateTemplate;

        VkShaderStageFlags pushConstantStages;
        VkPipelineBindPoint bindPoint;

        uint32_t pushConstantSize;
    };

    struct PipelineConfigs_vk
    {
        bool enableDepthTest{ true };
        bool enableDepthWrite{ true };
        VkCompareOp depthCompOp{ VK_COMPARE_OP_GREATER };
        VkCullModeFlags cullMode{ VK_CULL_MODE_BACK_BIT };
    };

    bool loadShader(Shader_vk& shader, VkDevice device, const char* path);

    VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const VkPipelineRenderingCreateInfo& renderInfo
        , const stl::vector<Shader_vk>& shaders, VkPipelineVertexInputStateCreateInfo* vtxInputState, const stl::vector<int> constants = {}, const PipelineConfigs_vk& pipeConfigs = {});
    VkPipeline createComputePipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const Shader_vk& shader, const stl::vector<int> constants = {});

    VkDescriptorSetLayout createDescSetLayout(VkDevice device, const stl::vector<Shader_vk>& shaders);
    VkPipelineLayout createPipelineLayout(VkDevice _device, VkDescriptorSetLayout _setLayout, VkDescriptorSetLayout _arrayLayout, VkShaderStageFlags _pushConstantStages, size_t _pushConstantSize);

    VkDescriptorPool createDescriptorPool(VkDevice _device);
    VkDescriptorSetLayout createDescArrayLayout(VkDevice _device, VkShaderStageFlags _stages);
    VkDescriptorSet createDescriptorSets(VkDevice _device, VkDescriptorSetLayout _layout, VkDescriptorPool _pool, uint32_t _descCount);

    Program_vk createProgram(VkDevice _device, VkPipelineBindPoint _bindingPoint, const stl::vector<Shader_vk>& _shaders, uint32_t _pushConstantSize = 0, VkDescriptorSetLayout _dsLayout = NULL);
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

        VkBufferView bufferView;

        DescriptorInfo()
        {
            bufferView = VK_NULL_HANDLE;
        }

        DescriptorInfo(VkBuffer _buffer, VkDeviceSize offset, VkDeviceSize range)
        {
            buffer.buffer = _buffer;
            buffer.offset = offset;
            buffer.range = range;

            bufferView = VK_NULL_HANDLE;
        }

        DescriptorInfo(VkBuffer _buffer)
        {
            buffer.buffer = _buffer;
            buffer.offset = 0;
            buffer.range = VK_WHOLE_SIZE;

            bufferView = VK_NULL_HANDLE;
        }

        DescriptorInfo(VkBuffer _buffer, VkBufferView _bufferView)
        {
            buffer.buffer = _buffer;
            buffer.offset = 0;
            buffer.range = VK_WHOLE_SIZE;

            bufferView = _bufferView;
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
}