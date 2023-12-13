#pragma once

#include "vkz_structs_inner.h"

namespace vkz
{

    struct Buffer_vk
    {
        uint16_t resId;

        VkBuffer buffer;
        VkDeviceMemory memory;
        void* data;
        size_t size;
    };

    struct BufAliasInfo_vk
    {
        uint16_t    resId;
        size_t      size;
    };

    Buffer_vk createBuffer(const BufferAliasInfo& info, const VkPhysicalDeviceMemoryProperties& _memProps, VkDevice _device, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memFlags);
    void createBuffer(std::vector<Buffer_vk>& _results, const std::vector<BufferAliasInfo> _infos, const VkPhysicalDeviceMemoryProperties& _memProps, VkDevice _device, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memFlags);
    void uploadBuffer(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const Buffer_vk& buffer, const Buffer_vk& scratch, const void* data, size_t size);
    void flushBuffer(VkDevice device, const Buffer_vk& buffer, uint32_t offset = 0);

    // destroy a list of buffers, which shares the same memory
    void destroyBuffer(const VkDevice _device, const std::vector<Buffer_vk>& _buffers);

    VkBufferMemoryBarrier2 bufferBarrier(VkBuffer buffer, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 dstStage);

    struct Image_vk
    {
        uint32_t ID;
        uint16_t resId;

        VkImage image;
        VkImageView imageView;
        VkDeviceMemory memory;

        uint32_t mipLevels;
        uint32_t width, height;
    };

    struct ImgAliasInfo_vk
    {
        uint16_t resId;
    };

    struct ImgInitProps
    {
        uint32_t level;

        uint32_t width;
        uint32_t height;
        uint32_t depth;

        VkFormat            format;
        VkImageUsageFlags   usage;
        VkImageType         type{ VK_IMAGE_TYPE_2D };
        VkImageLayout       layout{ VK_IMAGE_LAYOUT_GENERAL };
        VkImageViewType     viewType{ VK_IMAGE_VIEW_TYPE_2D };
    };

    Image_vk createImage(const ImageAliasInfo& info, const VkDevice _device, const VkPhysicalDeviceMemoryProperties& _memProps, const ImgInitProps& _initProps);
    void createImage(std::vector<Image_vk>& _results, const std::vector<ImageAliasInfo>& _infos, const VkDevice _device, const VkPhysicalDeviceMemoryProperties& _memProps, const ImgInitProps& _initProps);
    void uploadImage(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const Image_vk& image, const Buffer_vk& scratch, const void* data, size_t size, VkImageLayout layout, const uint32_t regionCount = 1, uint32_t mipLevels = 1);
    // destroy a list of buffers, which shares the same memory
    void destroyImage(const VkDevice _device, const std::vector<Image_vk>& _images);


    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

    VkImageMemoryBarrier2 imageBarrier(VkImage image, VkImageAspectFlagBits aspectMask, VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout, VkPipelineStageFlags2 dstStage);
    void pipelineBarrier(VkCommandBuffer cmdBuffer, VkDependencyFlags flags, size_t bufferBarrierCount, const VkBufferMemoryBarrier2* bufferBarriers, const uint32_t imageBarrierCount, const VkImageMemoryBarrier2* imageBarriers);

    uint32_t calculateMipLevelCount(uint32_t width, uint32_t height);

    VkSampler createSampler(VkDevice device, VkSamplerReductionMode reductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE);


}
