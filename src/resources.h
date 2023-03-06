#pragma once


struct Buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* data;
    size_t size;
};


void createBuffer(Buffer& result, const VkPhysicalDeviceMemoryProperties memoryProps, VkDevice device, size_t sz, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags);
void uploadBuffer(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const Buffer& buffer, const Buffer& scratch, const void* data, size_t size);
void destroyBuffer(const Buffer& buffer, VkDevice device);
VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format);

VkImageMemoryBarrier imageBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout);