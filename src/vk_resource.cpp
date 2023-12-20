#include "common.h"
#include "vk_resource.h"


#include <ktx.h>
#include <ktxvulkan.h>

#include <functional> // for hash
#include <string>

namespace vkz
{

    // Image first bit is 0
    // Buffer first bit is 1
    #define IMAGE_HASH_MASK 0x7FFFFFFF
    #define BUFFER_HASH_MASK 0xFFFFFFFF

    uint32_t hash32(VkImage image)
    {
        uint32_t hash = std::hash<VkImage>{}(image) & IMAGE_HASH_MASK;

        return hash;
    }

    uint32_t hash32(VkBuffer buffer)
    {
        uint32_t hash = std::hash<VkBuffer>{}(buffer) & BUFFER_HASH_MASK;

        return hash;
    }

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

    void createBuffer(std::vector<Buffer_vk>& _results, const std::vector<BufferAliasInfo> _infos, const VkPhysicalDeviceMemoryProperties& _memProps
            , const VkDevice _device, const VkBufferUsageFlags _usage, const VkMemoryPropertyFlags _memFlags)
    {
        if (_infos.empty())
        {
            return;
        }

        assert(_results.empty());

        uint32_t size = (uint32_t)_infos.size();
        std::vector<Buffer_vk> results(size);

        for (uint32_t ii = 0; ii < size; ++ii)
        {
            VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            createInfo.size = _infos[ii].size;
            createInfo.usage = _usage;

            Buffer_vk& buf = results[ii];

            VK_CHECK(vkCreateBuffer(_device, &createInfo, nullptr, &(buf.buffer)));
        }

        // allocate memory only for the first buffer
        VkBuffer& baseBuf = results[0].buffer;

        VkMemoryRequirements memoryReqs;
        vkGetBufferMemoryRequirements(_device, baseBuf, &memoryReqs);

        uint32_t memoryTypeIdx = selectMemoryType(_memProps, memoryReqs.memoryTypeBits, _memFlags);
        assert(memoryTypeIdx != ~0u);

        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memoryReqs.size;
        allocInfo.memoryTypeIndex = memoryTypeIdx;

        VkDeviceMemory memory = 0;
        VK_CHECK(vkAllocateMemory(_device, &allocInfo, nullptr, &memory));

        // all buffers share the same memory
        for (uint32_t ii = 0; ii < size; ++ii)
        {
            Buffer_vk& buf = results[ii];
            VK_CHECK(vkBindBufferMemory(_device, buf.buffer, memory, 0));
            buf.memory = memory;
            buf.size = _infos[ii].size;
            buf.resId = _infos[ii].bufId;
        }

        // map to local memory
        if (_memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            for (uint32_t ii = 0; ii < size; ++ii)
            {
                Buffer_vk& buf = results[ii];
                VK_CHECK(vkMapMemory(_device, memory, 0, buf.size, 0, &(buf.data)));
            }
        }

        _results = std::move(results);
    }

    Buffer_vk createBuffer(const BufferAliasInfo& info, const VkPhysicalDeviceMemoryProperties& _memProps, VkDevice _device, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memFlags)
    {
        std::vector<Buffer_vk> results;
        createBuffer(results, {info}, _memProps, _device, _usage, _memFlags);

        return results[0];
    }

    void uploadBuffer(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const Buffer_vk& buffer, const Buffer_vk& scratch, const void* data, size_t size)
    {
        assert(size > 0);
        assert(scratch.data);
        assert(scratch.size >= size);
    
        memcpy(scratch.data, data, size);


        VK_CHECK(vkResetCommandPool(device, cmdPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

        VkBufferCopy region = { 0, 0, VkDeviceSize(size) };
        vkCmdCopyBuffer(cmdBuffer, scratch.buffer, buffer.buffer, 1, &region);

        VkBufferMemoryBarrier2 copyBarrier = bufferBarrier(buffer.buffer,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 1, &copyBarrier, 0, 0);

        VK_CHECK(vkEndCommandBuffer(cmdBuffer));

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

        VK_CHECK(vkDeviceWaitIdle(device));
    }

    // only for non-coherent buffer 
    void flushBuffer(VkDevice device, const Buffer_vk& buffer, uint32_t offset /* = 0*/)
    {
        VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        range.memory = buffer.memory;
        range.size = VK_WHOLE_SIZE;
        range.offset = offset;

        VK_CHECK(vkFlushMappedMemoryRanges(device, 1, &range));
    }

    void destroyBuffer(const VkDevice _device, const std::vector<Buffer_vk>& _buffers)
    {
        assert(!_buffers.empty());
        
        // depends on this doc:
        //    If a memory object is mapped at the time it is freed, it is implicitly unmapped.
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
        Buffer_vk baseBuf = _buffers[0];
        vkFreeMemory(_device, baseBuf.memory, 0);

        for (const Buffer_vk& buf : _buffers)
        {
            vkDestroyBuffer(_device, buf.buffer, 0);
        }
    }

    VkBufferMemoryBarrier2 bufferBarrier(VkBuffer buffer, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 dstStage)
    {
        VkBufferMemoryBarrier2 result = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
        result.srcAccessMask = srcAccessMask;
        result.dstAccessMask = dstAccessMask;
        result.srcStageMask = srcStage;
        result.dstStageMask = dstStage;
        result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        result.buffer = buffer;
        result.offset = 0;
        result.size = VK_WHOLE_SIZE;

        return result;
    }


    void createImage(std::vector<Image_vk>& _results, const std::vector<ImageAliasInfo>& _infos, const VkDevice _device, const VkPhysicalDeviceMemoryProperties& _memProps, const ImgInitProps& _initProps)
    {
        if (_infos.empty())
        {
            return;
        }

        assert(_results.empty());

        uint32_t num = (uint32_t)_infos.size();
        std::vector<Image_vk> results(num);

        VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

        createInfo.imageType = _initProps.type;
        createInfo.format = _initProps.format;
        createInfo.extent = { _initProps.width, _initProps.height, _initProps.depth };
        createInfo.mipLevels = _initProps.level;
        createInfo.arrayLayers = (_initProps.viewType == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.usage = _initProps.usage;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (_initProps.viewType == VK_IMAGE_VIEW_TYPE_CUBE)
        {
            createInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        // create for aliases
        for (uint32_t ii = 0; ii < num; ++ii)
        {
            Image_vk& img = results[ii];
            VK_CHECK(vkCreateImage(_device, &createInfo, 0, &(img.image)));
        }


        VkImage baseImg = results[0].image;

        VkMemoryRequirements memoryReqs;
        vkGetImageMemoryRequirements(_device, baseImg, &memoryReqs);

        uint32_t memoryTypeIdx = selectMemoryType(_memProps, memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memoryReqs.size;
        allocInfo.memoryTypeIndex = memoryTypeIdx;

        VkDeviceMemory memory = 0;
        VK_CHECK(vkAllocateMemory(_device, &allocInfo, 0, &memory));

        for (uint32_t ii = 0; ii < num; ++ii)
        {
            Image_vk& img = results[ii];
            VK_CHECK(vkBindImageMemory(_device, img.image, memory, 0));
        }

        // setting for aliases
        for (uint32_t ii = 0; ii < num; ++ii)
        {
            Image_vk& img = results[ii];

            img.ID = hash32(img.image);
            img.resId = _infos[ii].imgId;
            img.imageView = createImageView(_device, img.image, _initProps.format, 0, _initProps.level, _initProps.viewType); // ImageView bind to the image handle it self, should create a new one for alias
            img.memory = memory;

            img.width = _initProps.width;
            img.height = _initProps.height;
            img.mipLevels = _initProps.level;
        }

        _results = std::move(results);
    }

    vkz::Image_vk createImage(const ImageAliasInfo& _info, const VkDevice _device, const VkPhysicalDeviceMemoryProperties& _memProps, const ImgInitProps& _initProps)
    {
        std::vector<Image_vk> results;
        createImage(results, {_info}, _device, _memProps, _initProps);

        return results[0];
    }

    void loadKtxFromFile(ktxTexture** texture, const char* path)
    {
        assert(path);

        ktxResult result = KTX_SUCCESS;
        result = ktxTexture_CreateFromNamedFile(path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture);
        assert(result == KTX_SUCCESS);
    }

    void uploadImage(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const Image_vk& image, const Buffer_vk& scratch, const void* data, size_t size, VkImageLayout layout, const uint32_t regionCount /*= 1*/, uint32_t mipLevels /*= 1*/)
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


        std::vector<VkBufferImageCopy> regions;
        for (uint32_t face = 0; face < regionCount; ++face)
        {
            for (uint32_t level = 0; level < mipLevels; ++level)
            {

            }
        }

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
        useBarrier[0].newLayout = layout;
        useBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        useBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        useBarrier[0].image = image.image;
        useBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        useBarrier[0].subresourceRange.levelCount = 1;
        useBarrier[0].subresourceRange.layerCount = regionCount;
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, useBarrier);

        VK_CHECK(vkEndCommandBuffer(cmdBuffer));

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

        VK_CHECK(vkDeviceWaitIdle(device));
    }

    void destroyImage(const VkDevice _device, const std::vector<Image_vk>& _images)
    {
        assert(!_images.empty());

        // depends on this doc:
        //    If a memory object is mapped at the time it is freed, it is implicitly unmapped.
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
        Image_vk baseImg = _images[0];
        vkFreeMemory(_device, baseImg.memory, nullptr);

        for (const Image_vk& img : _images)
        {
            vkDestroyImageView(_device, img.imageView, nullptr);
            vkDestroyImage(_device, img.image, nullptr);
        }
    }

    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, uint32_t baseMipLevel, uint32_t levelCount, VkImageViewType viewType /*= VK_IMAGE_VIEW_TYPE_2D*/)
    {
        VkImageAspectFlags aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = image;
        createInfo.viewType = viewType;
        createInfo.format = format;
        createInfo.subresourceRange.aspectMask = aspectMask;
        createInfo.subresourceRange.baseMipLevel = baseMipLevel;
        createInfo.subresourceRange.levelCount = levelCount;
        createInfo.subresourceRange.layerCount = ((viewType == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1);

        VkImageView imageView = 0;
        VK_CHECK(vkCreateImageView(device, &createInfo, 0, &imageView));
        return imageView;
    }

    VkImageMemoryBarrier2 imageBarrier(VkImage image, VkImageAspectFlagBits aspectMask, 
        VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout, VkPipelineStageFlags2 srcStage,
        VkAccessFlags2 dstAccessMask,  VkImageLayout newLayout, VkPipelineStageFlags2 dstStage)
    {
        VkImageMemoryBarrier2 result = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    
        result.srcAccessMask = srcAccessMask;
        result.dstAccessMask = dstAccessMask;
        result.oldLayout = oldLayout;
        result.newLayout = newLayout;
        result.srcStageMask = srcStage;
        result.dstStageMask = dstStage;
        result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        result.image = image;
        result.subresourceRange.aspectMask = aspectMask;
        result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        return result;
    }

    void pipelineBarrier(VkCommandBuffer cmdBuffer, VkDependencyFlags flags, size_t bufferBarrierCount, const VkBufferMemoryBarrier2* bufferBarriers, const uint32_t imageBarrierCount, const VkImageMemoryBarrier2* imageBarriers)
    {
        VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR };
        dependencyInfo.dependencyFlags = flags;
        dependencyInfo.bufferMemoryBarrierCount = (uint32_t)bufferBarrierCount;
        dependencyInfo.pBufferMemoryBarriers = bufferBarriers;
        dependencyInfo.imageMemoryBarrierCount = (uint32_t)imageBarrierCount;
        dependencyInfo.pImageMemoryBarriers = imageBarriers;
    
        vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
    }

    uint32_t calculateMipLevelCount(uint32_t width, uint32_t height)
    {
        uint32_t result = 0;
        while (width > 1 || height > 1 )
        {
            result++;
            width >>= 1;
            height >>= 1;
        }

        return result;
    }

    VkSampler createSampler(VkDevice device, VkSamplerReductionMode reductionMode /*= VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE*/)
    {
        VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

        createInfo.magFilter = VK_FILTER_LINEAR;
        createInfo.minFilter = VK_FILTER_LINEAR;
        createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.minLod = 0.f;
        createInfo.maxLod = 16.f; // required by occlusion culling, or tiny object will just culled since greater pyramid lod does not exist;

        VkSamplerReductionModeCreateInfoEXT createInfoReduction = { VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT };
        if (reductionMode != VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE)
        {
            createInfoReduction.reductionMode = reductionMode;
            createInfo.pNext = &createInfoReduction;
        }

        VkSampler sampler = 0;
        VK_CHECK(vkCreateSampler(device, &createInfo, 0, &sampler));

        return sampler;
    }


} // namespace vkz