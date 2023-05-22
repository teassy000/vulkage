#pragma once

struct Buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* data;
    size_t size;
};


void createBuffer(Buffer& result, const VkPhysicalDeviceMemoryProperties& memoryProps, VkDevice device, size_t sz, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags);
void uploadBuffer(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const Buffer& buffer, const Buffer& scratch, const void* data, size_t size);
void flushBuffer(VkDevice device, const Buffer& buffer, uint32_t offset = 0);
void destroyBuffer(VkDevice device, const Buffer& buffer);

VkBufferMemoryBarrier2 bufferBarrier(VkBuffer buffer, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 dstStage);


struct Image
{
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

void createImage(Image& result, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProps, const VkFormat format, const ImgInitProps createInfo);
void loadTexture2DFromFile(Image& tex2d, const char* path, VkDevice device, VkCommandPool cmdPool, VkQueue queue, const VkPhysicalDeviceMemoryProperties& memoryProps, VkFormat format, VkImageUsageFlags usage, VkImageLayout layout);
void loadTexture2DArrayFromFile(Image& tex2dArray, const char* path, VkDevice device, VkCommandPool cmdPool, VkQueue queue, const VkPhysicalDeviceMemoryProperties& memoryProps, VkFormat format, VkImageUsageFlags usage);
void loadTextureCubeFromFile(Image& texCube, const char* path, VkDevice device, VkCommandPool cmdPool, VkQueue queue, const VkPhysicalDeviceMemoryProperties& memoryProps, VkFormat format, VkImageUsageFlags usage, VkImageLayout layout);
void uploadImage(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const Image& image, const Buffer& scratch, const void* data, size_t size, VkImageLayout layout, const uint32_t regionCount = 1, uint32_t mipLevels = 1);
void destroyImage(VkDevice device, const Image& image);
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

VkImageMemoryBarrier2 imageBarrier(VkImage image, VkImageAspectFlagBits aspectMask, VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout, VkPipelineStageFlags2 dstStage);
void pipelineBarrier(VkCommandBuffer cmdBuffer, VkDependencyFlags flags, size_t bufferBarrierCount, const VkBufferMemoryBarrier2* bufferBarriers, const uint32_t imageBarrierCount, const VkImageMemoryBarrier2* imageBarriers);

uint32_t calculateMipLevelCount(uint32_t width, uint32_t height);

VkSampler createSampler(VkDevice device, VkSamplerReductionMode reductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE);