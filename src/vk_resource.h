#pragma once

//typedef unsigned int uint32_t;

namespace vkz
{

    struct VK_Buffer
    {
        uint16_t resId;

        VkBuffer buffer;
        VkDeviceMemory memory;
        void* data;
        size_t size;
    };

    struct VK_BufAliasInfo
    {
        uint16_t    resId;
        size_t      size;
    };


    using BufferList = std::initializer_list<VK_Buffer* const>;
    using SizeList = std::initializer_list<size_t>;

    void createBuffer(VK_Buffer& result, const VkPhysicalDeviceMemoryProperties& _memProps, VkDevice _device, size_t sz, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memFlags, BufferList = {});
    void createBuffer(std::vector<VK_Buffer>& _results, const std::vector<VK_BufAliasInfo> _infos, const VkPhysicalDeviceMemoryProperties& _memProps, VkDevice _device, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memFlags);
    void uploadBuffer(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const VK_Buffer& buffer, const VK_Buffer& scratch, const void* data, size_t size);
    void flushBuffer(VkDevice device, const VK_Buffer& buffer, uint32_t offset = 0);
    void destroyBuffer(VkDevice device, const VK_Buffer& buffer, BufferList = {});

    VkBufferMemoryBarrier2 bufferBarrier(VkBuffer buffer, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 dstStage);

    struct VK_Image
    {
        uint32_t ID;

        VkImage image;
        VkImageView imageView;
        VkDeviceMemory memory;

        uint32_t mipLevels;
        uint32_t width, height;
    };

    struct ImgInitProps
    {
        uint32_t mipLevels;
        uint32_t width, height;

        VkImageUsageFlags usage;
        VkImageType       type{ VK_IMAGE_TYPE_2D };
        VkImageLayout     layout{ VK_IMAGE_LAYOUT_GENERAL };
        VkImageViewType   viewType{ VK_IMAGE_VIEW_TYPE_2D };
    };

    using ImageAliases = std::initializer_list<VK_Image* const>;

    void createImage(VK_Image& result, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProps, const VkFormat format, const ImgInitProps createInfo, ImageAliases = {}, const uint32_t aliasCount = 0);
    void loadTexture2DFromFile(VK_Image& tex2d, const char* path, VkDevice device, VkCommandPool cmdPool, VkQueue queue, const VkPhysicalDeviceMemoryProperties& memoryProps, VkFormat format, VkImageUsageFlags usage, VkImageLayout layout);
    void loadTexture2DArrayFromFile(VK_Image& tex2dArray, const char* path, VkDevice device, VkCommandPool cmdPool, VkQueue queue, const VkPhysicalDeviceMemoryProperties& memoryProps, VkFormat format, VkImageUsageFlags usage);
    void loadTextureCubeFromFile(VK_Image& texCube, const char* path, VkDevice device, VkCommandPool cmdPool, VkQueue queue, const VkPhysicalDeviceMemoryProperties& memoryProps, VkFormat format, VkImageUsageFlags usage, VkImageLayout layout);
    void uploadImage(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const VK_Image& image, const VK_Buffer& scratch, const void* data, size_t size, VkImageLayout layout, const uint32_t regionCount = 1, uint32_t mipLevels = 1);
    void destroyImage(VkDevice device, const VK_Image& image, ImageAliases = {});
    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

    VkImageMemoryBarrier2 imageBarrier(VkImage image, VkImageAspectFlagBits aspectMask, VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout, VkPipelineStageFlags2 dstStage);
    void pipelineBarrier(VkCommandBuffer cmdBuffer, VkDependencyFlags flags, size_t bufferBarrierCount, const VkBufferMemoryBarrier2* bufferBarriers, const uint32_t imageBarrierCount, const VkImageMemoryBarrier2* imageBarriers);

    uint32_t calculateMipLevelCount(uint32_t width, uint32_t height);

    VkSampler createSampler(VkDevice device, VkSamplerReductionMode reductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE);


}
