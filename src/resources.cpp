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

void createBuffer(Buffer& result, const VkPhysicalDeviceMemoryProperties memoryProps, VkDevice device, size_t sz, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags)
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

void destroyBuffer(const Buffer& buffer, VkDevice device)
{
    // depends on the doc:
    //    If a memory object is mapped at the time it is freed, it is implicitly unmapped.
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
    vkFreeMemory(device, buffer.memory, 0);
    vkDestroyBuffer(device, buffer.buffer, 0);
}


VkImageView createImageView(VkDevice device, VkImage image, VkFormat format)
{
    VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView = 0;
    VK_CHECK(vkCreateImageView(device, &createInfo, 0, &imageView));
    return imageView;
}


VkImageMemoryBarrier imageBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier result = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

    result.srcAccessMask = srcAccessMask;
    result.dstAccessMask = dstAccessMask;
    result.oldLayout = oldLayout;
    result.newLayout = newLayout;
    result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.image = image;
    result.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return result;
}
