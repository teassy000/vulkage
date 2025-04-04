#pragma once
#include "common.h"
#include "kage_rhi_vk.h"

namespace kage { namespace vk
{
    struct Shader_vk
    {
        VkShaderModule module;
        VkShaderStageFlagBits stage;

        VkDescriptorType pushResTypes[32];
        uint32_t pushResMask;
        
        VkDescriptorType nonPushResTypes[32];
        uint32_t nonPushResCount[32];
        uint32_t nonPushResMask;

        uint32_t localSizeX;
        uint32_t localSizeY;
        uint32_t localSizeZ;

        uint32_t nonPushDescCount;
        bool usesPushConstants;
        bool usesBindless;
        bool hasNonPushDesc;
    };

    struct Program_vk
    {
        VkPipelineLayout        layout;
        VkDescriptorSetLayout   pushSetLayout;
        VkDescriptorSetLayout   nonPushSetLayout;
        VkDescriptorSetLayout   bindlessLayout;

        VkDescriptorUpdateTemplate updateTemplate;
        VkDescriptorSet nonPushDescSet;

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
        VkPolygonMode polygonMode{ VK_POLYGON_MODE_FILL };
    };

    bool loadShader(Shader_vk& shader, VkDevice device, const char* path);

    VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const VkPipelineRenderingCreateInfo& renderInfo
        , const stl::vector<Shader_vk>& shaders, VkPipelineVertexInputStateCreateInfo* vtxInputState, const stl::vector<int> constants = {}, const PipelineConfigs_vk& pipeConfigs = {});
    VkPipeline createComputePipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const Shader_vk& shader, const stl::vector<int> constants = {});

    VkDescriptorSetLayout createDescSetLayout(VkDevice device, const stl::vector<Shader_vk>& shaders, bool _push = true);
    VkPipelineLayout createPipelineLayout(VkDevice _device, uint32_t _layoutCount, VkDescriptorSetLayout* _layouts, VkShaderStageFlags _pushConstantStages, size_t _pushConstantSize);

    VkDescriptorPool createDescriptorPool(VkDevice _device);
    VkDescriptorSetLayout createDescArrayLayout(VkDevice _device, VkShaderStageFlags _stages);
    VkDescriptorSet createDescriptorSet(VkDevice _device, VkDescriptorSetLayout _layout, VkDescriptorPool _pool, uint32_t _descCount = 0, bool _bindless = false);

    Program_vk createProgram(VkDevice _device, VkPipelineBindPoint _bindingPoint, const stl::vector<Shader_vk>& _shaders, uint32_t _pushConstantSize, VkDescriptorSetLayout _dsLayout, VkDescriptorPool _pool);
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