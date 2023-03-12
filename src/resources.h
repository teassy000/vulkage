#pragma once

struct Buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* data;
    size_t size;
};

struct Image
{
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory memory;

    uint32_t width, height;
};

void createBuffer(Buffer& result, const VkPhysicalDeviceMemoryProperties& memoryProps, VkDevice device, size_t sz, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags);
void uploadBuffer(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const Buffer& buffer, const Buffer& scratch, const void* data, size_t size);
void flushBuffer(VkDevice device, const Buffer& buffer, uint32_t offset = 0);
void destroyBuffer(VkDevice device, const Buffer& buffer);
VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);

void createImage(Image& result, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage);
void uploadImage(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const Image& image, const Buffer& scratch, const void* data, size_t size);
void destroyImage(VkDevice device, const Image& image);
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount);
VkImageMemoryBarrier imageBarrier(VkImage image, VkImageAspectFlagBits aspectMask, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout);

VkSampler createSampler(VkDevice device);