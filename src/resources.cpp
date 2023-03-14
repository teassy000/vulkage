#include "common.h"
#include "resources.h"

uint32_t selectMemoryType(const VkPhysicalDeviceMemoryProperties& memoryProps, uint32_t memoryTypeBits, VkMemoryPropertyFlags flags)
{
    for (uint32_t i = 0; i < memoryProps.memoryTypeCount; ++i) {
        if ((memoryTypeBits & (1 << i)) != 0 && (memoryProps.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    assert(!"No compatible memory type found!\n");
    return ~0u;
}

void createBuffer(Buffer& result, const VkPhysicalDeviceMemoryProperties& memoryProps, VkDevice device, size_t sz, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags)
{
    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = sz;
    createInfo.usage = usage;

    VkBuffer buffer = 0;
    VK_CHECK(vkCreateBuffer(device, &createInfo, 0, &buffer));

    VkMemoryRequirements memoryReqs;
    vkGetBufferMemoryRequirements(device, buffer, &memoryReqs);

    uint32_t memoryTypeIdx = selectMemoryType(memoryProps, memoryReqs.memoryTypeBits, memFlags);
    assert(memoryTypeIdx != ~0u);

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memoryReqs.size;
    allocInfo.memoryTypeIndex = memoryTypeIdx;

    VkDeviceMemory memory = 0;
    VK_CHECK(vkAllocateMemory(device, &allocInfo, 0, &memory));

    VK_CHECK(vkBindBufferMemory(device, buffer, memory, 0));

    void* data = 0;
    if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        VK_CHECK(vkMapMemory(device, memory, 0, sz, 0, &data));
    }

    result.buffer = buffer;
    result.memory = memory;
    result.data = data;
    result.size = sz;
}

void uploadBuffer(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const Buffer& buffer, const Buffer& scratch, const void* data, size_t size)
{
    assert(scratch.data);
    assert(scratch.size >= size);
    
    memcpy(scratch.data, data, size);


    VK_CHECK(vkResetCommandPool(device, cmdPool, 0));

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

    VkBufferCopy region = { 0, 0, VkDeviceSize(size) };
    vkCmdCopyBuffer(cmdBuffer, scratch.buffer, buffer.buffer, 1, &region);

    VkBufferMemoryBarrier copyBarrier = bufferBarrier(buffer.buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
        , VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &copyBarrier, 0, 0);

    VK_CHECK(vkEndCommandBuffer(cmdBuffer));

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

    VK_CHECK(vkDeviceWaitIdle(device));
}

// only for non-coherent buffer 
void flushBuffer(VkDevice device, const Buffer& buffer, uint32_t offset /* = 0*/)
{
    VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
    range.memory = buffer.memory;
    range.size = buffer.size;
    range.offset = offset;

    VK_CHECK(vkFlushMappedMemoryRanges(device, 1, &range));
}

void destroyBuffer(VkDevice device, const Buffer& buffer)
{
    // depends on the doc:
    //    If a memory object is mapped at the time it is freed, it is implicitly unmapped.
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
    vkFreeMemory(device, buffer.memory, 0);
    vkDestroyBuffer(device, buffer.buffer, 0);
}


VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
{
    VkBufferMemoryBarrier result = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
    result.srcAccessMask = srcAccessMask;
    result.dstAccessMask = dstAccessMask;
    result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.buffer = buffer;
    result.offset = 0;
    result.size = VK_WHOLE_SIZE;

    return result;
}


void createImage(Image& result, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage)
{
    VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = format;
    createInfo.extent = { width, height, 1u };
    createInfo.mipLevels = mipLevels;
    createInfo.arrayLayers = 1; // according to validation: arrayLayers must greater than 0;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = usage;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage image = 0;
    VK_CHECK(vkCreateImage(device, &createInfo, 0, &image));

    VkMemoryRequirements memoryReqs;
    vkGetImageMemoryRequirements(device, image, &memoryReqs);

    uint32_t memoryTypeIdx = selectMemoryType(memoryProperties, memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memoryReqs.size;
    allocInfo.memoryTypeIndex = memoryTypeIdx;

    VkDeviceMemory memory = 0;
    VK_CHECK(vkAllocateMemory(device, &allocInfo, 0, &memory));

    VK_CHECK(vkBindImageMemory(device, image, memory, 0));

    result.image = image;
    result.imageView = createImageView(device, image, format, 0, mipLevels);
    result.memory = memory;
    result.width = width;
    result.height = height;
}

void uploadImage(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const Image& image, const Buffer& scratch, const void* data, size_t size)
{
    assert(scratch.data);
    assert(scratch.size >= size);
    
    memcpy(scratch.data, data, size);

    VK_CHECK(vkResetCommandPool(device, cmdPool, 0));

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

    VkImageMemoryBarrier copyBarrier[1] = {};
    copyBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    copyBarrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    copyBarrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    copyBarrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copyBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyBarrier[0].image = image.image;
    copyBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyBarrier[0].subresourceRange.levelCount = 1;
    copyBarrier[0].subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, copyBarrier);

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = image.width;
    region.imageExtent.height = image.height;
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(cmdBuffer, scratch.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier useBarrier[1] = {};
    useBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    useBarrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    useBarrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    useBarrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    useBarrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    useBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    useBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    useBarrier[0].image = image.image;
    useBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    useBarrier[0].subresourceRange.levelCount = 1;
    useBarrier[0].subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, useBarrier);

    VK_CHECK(vkEndCommandBuffer(cmdBuffer));

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

    VK_CHECK(vkDeviceWaitIdle(device));
}

void destroyImage(VkDevice device, const Image& image)
{
    vkFreeMemory(device, image.memory, 0);
    vkDestroyImageView(device, image.imageView, 0);
    vkDestroyImage(device, image.image, 0);
}


VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount)
{
    VkImageAspectFlags aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = aspectMask;
    createInfo.subresourceRange.baseMipLevel = baseMipLevel;
    createInfo.subresourceRange.levelCount = levelCount;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView = 0;
    VK_CHECK(vkCreateImageView(device, &createInfo, 0, &imageView));
    return imageView;
}


VkImageMemoryBarrier imageBarrier(VkImage image, VkImageAspectFlagBits aspectMask, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier result = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    
    result.srcAccessMask = srcAccessMask;
    result.dstAccessMask = dstAccessMask;
    result.oldLayout = oldLayout;
    result.newLayout = newLayout;
    result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.image = image;
    result.subresourceRange.aspectMask = aspectMask;
    result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return result;
}

VkSampler createSampler(VkDevice device)
{
    VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    VkSampler sampler = 0;
    VK_CHECK(vkCreateSampler(device, &createInfo, 0, &sampler));

    return sampler;
}
