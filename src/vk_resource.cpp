#include "common.h"
#include "vk_resource.h"


#include <ktx.h>
#include <ktxvulkan.h>

#include <functional> // for hash
#include <string>
#include "profiler.h"

namespace kage
{
    VkDeviceMemory allocVkMemory(const VkDevice _device, size_t _size, uint32_t _memTypeIdx)
    {
        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = _size;
        allocInfo.memoryTypeIndex = _memTypeIdx;

        VkDeviceMemory memory = 0;
        VK_CHECK(vkAllocateMemory(_device, &allocInfo, nullptr, &memory));
        
        VKZ_ProfAlloc((void*)memory, _size);

        return memory;
    }

    void freeVkMemory(const VkDevice _device, VkDeviceMemory _mem)
    {
        vkFreeMemory(_device, _mem, nullptr);
        VKZ_ProfFree((void*)_mem);
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

    void createBuffer(stl::vector<Buffer_vk>& _results, const stl::vector<BufferAliasInfo> _infos, const VkPhysicalDeviceMemoryProperties& _memProps
            , const VkDevice _device, const VkBufferUsageFlags _usage, const VkMemoryPropertyFlags _memFlags)
    {
        if (_infos.empty())
        {
            return;
        }

        assert(_results.empty());
        
        uint32_t size = (uint32_t)_infos.size();
        stl::vector<Buffer_vk> results(size);

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

        VkDeviceMemory memory = allocVkMemory(_device, memoryReqs.size, memoryTypeIdx);

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
        stl::vector<Buffer_vk> results;
        stl::vector<BufferAliasInfo> infos{1, info };
        createBuffer(results, infos, _memProps, _device, _usage, _memFlags);

        return results[0];
    }

    void fillBuffer(VkDevice _device, VkCommandPool _cmdPool, VkCommandBuffer _cmdBuffer, VkQueue _queue, const Buffer_vk& _buffer, uint32_t _value, size_t _size)
    {
        assert(_size > 0);

        VK_CHECK(vkResetCommandPool(_device, _cmdPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(_cmdBuffer, &beginInfo));

        vkCmdFillBuffer(_cmdBuffer, _buffer.buffer, 0, _size, _value);

        VkBufferMemoryBarrier2 copyBarrier = bufferBarrier(_buffer.buffer,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        pipelineBarrier(_cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 1, &copyBarrier, 0, 0);

        VK_CHECK(vkEndCommandBuffer(_cmdBuffer));

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_cmdBuffer;

        VK_CHECK(vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE));

        VK_CHECK(vkDeviceWaitIdle(_device));
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

    void destroyBuffer(const VkDevice _device, const stl::vector<Buffer_vk>& _buffers)
    {
        assert(!_buffers.empty());
        
        // depends on this doc:
        //    If a memory object is mapped at the time it is freed, it is implicitly unmapped.
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
        Buffer_vk baseBuf = _buffers[0];
        freeVkMemory(_device, baseBuf.memory);

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


    void createImage(stl::vector<Image_vk>& _results, const stl::vector<ImageAliasInfo>& _infos, const VkDevice _device, const VkPhysicalDeviceMemoryProperties& _memProps, const ImgInitProps_vk& _initProps)
    {
        if (_infos.empty())
        {
            return;
        }

        assert(_results.empty());

        uint32_t num = (uint32_t)_infos.size();
        stl::vector<Image_vk> results(num);

        VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

        createInfo.imageType = _initProps.type;
        createInfo.format = _initProps.format;
        createInfo.extent = { _initProps.width, _initProps.height, _initProps.depth };
        createInfo.mipLevels = _initProps.numMips;
        createInfo.arrayLayers = _initProps.numLayers;
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
        VkDeviceMemory memory = allocVkMemory(_device, memoryReqs.size, memoryTypeIdx);

        for (uint32_t ii = 0; ii < num; ++ii)
        {
            Image_vk& img = results[ii];
            VK_CHECK(vkBindImageMemory(_device, img.image, memory, 0));
        }

        // setting for aliases
        for (uint32_t ii = 0; ii < num; ++ii)
        {
            Image_vk& img = results[ii];

            img.resId = _infos[ii].imgId;
            img.defalutImgView = createImageView(_device, img.image, _initProps.format, 0, _initProps.numMips, _initProps.viewType); // ImageView bind to the image handle it self, should create a new one for alias
            img.memory = memory;

            img.width = _initProps.width;
            img.height = _initProps.height;

            img.aspectMask = _initProps.aspectMask;
            img.format = _initProps.format;

            img.numMips = _initProps.numMips;
            img.numLayers = _initProps.numLayers;
        }

        _results = std::move(results);
    }

    kage::Image_vk createImage(const ImageAliasInfo& _info, const VkDevice _device, const VkPhysicalDeviceMemoryProperties& _memProps, const ImgInitProps_vk& _initProps)
    {
        stl::vector<Image_vk> results;
        createImage(results, {1, _info}, _device, _memProps, _initProps);
        assert(!results.empty());

        return results[0];
    }

    void loadKtxFromFile(ktxTexture** texture, const char* path)
    {
        assert(path);

        ktxResult result = KTX_SUCCESS;
        result = ktxTexture_CreateFromNamedFile(path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture);
        assert(result == KTX_SUCCESS);
    }


    void destroyImage(const VkDevice _device, const stl::vector<Image_vk>& _images)
    {
        assert(!_images.empty());


        // If a memory object is mapped at the time it is freed, it is implicitly unmapped.
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
        
        Image_vk baseImg = _images[0];
        freeVkMemory(_device, baseImg.memory);

        for (const Image_vk& img : _images)
        {
            vkDestroyImageView(_device, img.defalutImgView, nullptr);
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

    VkImageMemoryBarrier2 imageBarrier(VkImage image, VkImageAspectFlags aspectMask,
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