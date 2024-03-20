#pragma once

#include "vkz_inner.h"

namespace vkz
{

    struct Buffer_vk
    {
        uint16_t resId;

        VkBuffer buffer;
        VkDeviceMemory memory;
        void* data;
        size_t size;
        uint32_t fillVal;
    };

    Buffer_vk createBuffer(const BufferAliasInfo& info, const VkPhysicalDeviceMemoryProperties& _memProps, VkDevice _device, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memFlags);
    void createBuffer(stl::vector<Buffer_vk>& _results, const stl::vector<BufferAliasInfo> _infos, const VkPhysicalDeviceMemoryProperties& _memProps, VkDevice _device, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memFlags);

    void fillBuffer(VkDevice _device, VkCommandPool _cmdPool, VkCommandBuffer _cmdBuffer, VkQueue _queue, const Buffer_vk& _buffer, uint32_t _value, size_t _size);
    void flushBuffer(VkDevice device, const Buffer_vk& buffer, uint32_t offset = 0);

    // destroy a list of buffers, which shares the same memory
    void destroyBuffer(const VkDevice _device, const stl::vector<Buffer_vk>& _buffers);

    VkBufferMemoryBarrier2 bufferBarrier(VkBuffer buffer, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 dstStage);

    struct Image_vk
    {
        uint32_t ID;
        uint16_t resId;

        VkImage image;
        VkImageView defalutImgView;
        VkDeviceMemory memory;
        VkImageAspectFlags  aspectMask;
        VkFormat            format;

        uint32_t numMips;
        uint32_t numLayers;
        uint32_t width, height;
    };

    struct ImgAliasInfo_vk
    {
        uint16_t resId;
    };

    struct ImgInitProps_vk
    {
        uint32_t numMips;
        uint32_t numLayers;

        uint32_t width;
        uint32_t height;
        uint32_t depth;

        VkFormat            format;
        VkImageUsageFlags   usage;
        VkImageType         type{ VK_IMAGE_TYPE_2D };
        VkImageLayout       layout{ VK_IMAGE_LAYOUT_GENERAL };
        VkImageViewType     viewType{ VK_IMAGE_VIEW_TYPE_2D };
        VkImageAspectFlags  aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT };
    };

    Image_vk createImage(const ImageAliasInfo& info, const VkDevice _device, const VkPhysicalDeviceMemoryProperties& _memProps, const ImgInitProps_vk& _initProps);
    void createImage(stl::vector<Image_vk>& _results, const stl::vector<ImageAliasInfo>& _infos, const VkDevice _device, const VkPhysicalDeviceMemoryProperties& _memProps, const ImgInitProps_vk& _initProps);
    // destroy a list of buffers, which shares the same memory
    void destroyImage(const VkDevice _device, const stl::vector<Image_vk>& _images);


    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

    VkImageMemoryBarrier2 imageBarrier(VkImage image, VkImageAspectFlags aspectMask, VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout, VkPipelineStageFlags2 dstStage);
    void pipelineBarrier(VkCommandBuffer cmdBuffer, VkDependencyFlags flags, size_t bufferBarrierCount, const VkBufferMemoryBarrier2* bufferBarriers, const uint32_t imageBarrierCount, const VkImageMemoryBarrier2* imageBarriers);

    uint32_t calculateMipLevelCount(uint32_t width, uint32_t height);

    VkSampler createSampler(VkDevice device, VkSamplerReductionMode reductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE);


}
