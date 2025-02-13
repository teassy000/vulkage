#include "common.h"
#include "vk_resource.h"


#include <ktx.h>
#include <ktxvulkan.h>

#include <functional> // for hash
#include <string>
#include "profiler.h"
#include "rhi_context_vk.h"

namespace kage { namespace vk
{
    extern RHIContext_vk* s_renderVK;

    VkDeviceMemory allocVkMemory(
        const VkDevice _device
        , size_t _size
        , uint32_t _memTypeIdx
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = _size;
        allocInfo.memoryTypeIndex = _memTypeIdx;

        VkDeviceMemory memory = 0;
        VK_CHECK(vkAllocateMemory(_device, &allocInfo, nullptr, &memory));
        
        KG_ProfAlloc((void*)memory, _size);

        return memory;
    }


    void freeVkMemory(
        const VkDevice _device
        , VkDeviceMemory _mem
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        vkFreeMemory(_device, _mem, nullptr);
        KG_ProfFree((void*)_mem);
    }


    uint32_t selectMemoryType(
        const VkPhysicalDeviceMemoryProperties& _props
        , uint32_t _typeBits
        , VkMemoryPropertyFlags _flags
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        for (uint32_t i = 0; i < _props.memoryTypeCount; ++i) {
            if ((_typeBits & (1 << i)) != 0 && (_props.memoryTypes[i].propertyFlags & _flags) == _flags) {
                return i;
            }
        }

        assert(!"No compatible memory type found!\n");
        return ~0u;
    }


    void createBuffer(
        stl::vector<Buffer_vk>& _results
        , const stl::vector<BufferAliasInfo> _infos
        , const VkBufferUsageFlags _usage
        , const VkMemoryPropertyFlags _memFlags
        , const VkFormat _format /* = VK_FORMAT_UNDEFINED*/
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        if (_infos.empty())
        {
            return;
        }

        const VkPhysicalDeviceMemoryProperties& memProps = s_renderVK->m_memProps;
        const VkDevice device = s_renderVK->m_device;
        const VkPhysicalDeviceLimits& limits = s_renderVK->m_phyDeviceProps.limits;
        const VkPhysicalDevice& phyDevice = s_renderVK->m_physicalDevice;

        // process 
        _results.clear();

        uint32_t infoCount = (uint32_t)_infos.size();
        stl::vector<Buffer_vk> results(infoCount);

        if (_format != VK_FORMAT_UNDEFINED)
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(phyDevice, _format, &formatProperties);
            if (!(formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))
            {
                message(error, "unsupported buffer format for texel buffer!");
            }
        }


        for (uint32_t ii = 0; ii < infoCount; ++ii)
        {
            const uint32_t align = uint32_t(limits.nonCoherentAtomSize);
            const uint32_t alignSize = bx::strideAlign(_infos[ii].size, align);

            VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            createInfo.size = alignSize;
            createInfo.usage = _usage;

            Buffer_vk& buf = results[ii];

            VK_CHECK(vkCreateBuffer(device, &createInfo, nullptr, &(buf.buffer)));

            buf.size = alignSize;
            buf.resId = _infos[ii].bufId;
            buf.format = _format;
        }

        // allocate memory only for the first buffer
        VkBuffer& baseBuf = results[0].buffer;

        VkMemoryRequirements memoryReqs;
        vkGetBufferMemoryRequirements(device, baseBuf, &memoryReqs);

        uint32_t memoryTypeIdx = selectMemoryType(memProps, memoryReqs.memoryTypeBits, _memFlags);
        assert(memoryTypeIdx != ~0u);

        VkDeviceMemory memory = allocVkMemory(device, memoryReqs.size, memoryTypeIdx);

        // all buffers share the same memory
        for (uint32_t ii = 0; ii < infoCount; ++ii)
        {
            Buffer_vk& buf = results[ii];
            VK_CHECK(vkBindBufferMemory(device, buf.buffer, memory, 0));
            buf.memory = memory;
        }

        // map to local memory
        if (_memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            for (uint32_t ii = 0; ii < infoCount; ++ii)
            {
                Buffer_vk& buf = results[ii];
                VK_CHECK(vkMapMemory(device, memory, 0, buf.size, 0, &(buf.data)));
            }
        }

        _results = std::move(results);
    }


    Buffer_vk createBuffer(
        const BufferAliasInfo& _info
        , VkBufferUsageFlags _usage
        , VkMemoryPropertyFlags _memFlags
        , VkFormat _format /* = VK_FORMAT_UNDEFINED*/
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        stl::vector<Buffer_vk> results;
        stl::vector<BufferAliasInfo> infos{1, _info };
        createBuffer(results, infos, _usage, _memFlags, _format);

        return results[0];
    }

    // only for non-coherent buffer 
    void flushBuffer(
        const Buffer_vk& _buffer
        , uint32_t _offset /*= 0 */
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        const VkDevice device = s_renderVK->m_device;

        VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        range.memory = _buffer.memory;
        range.size = VK_WHOLE_SIZE;
        range.offset = _offset;

        VK_CHECK(vkFlushMappedMemoryRanges(device, 1, &range));
    }


    void destroyBuffer(
        const stl::vector<Buffer_vk>& _buffers
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        assert(!_buffers.empty());

        const VkDevice device = s_renderVK->m_device;
        
        // depends on this doc:
        //    If a memory object is mapped at the time it is freed, it is implicitly unmapped.
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
        Buffer_vk baseBuf = _buffers[0];
        freeVkMemory(device, baseBuf.memory);

        for (const Buffer_vk& buf : _buffers)
        {
            vkDestroyBuffer(device, buf.buffer, 0);
        }
    }

    VkBufferMemoryBarrier2 bufferBarrier(
        VkBuffer _buffer
        , VkAccessFlags2 _srcAccessMask
        , VkPipelineStageFlags2 _srcStage
        , VkAccessFlags2 _dstAccessMask
        , VkPipelineStageFlags2 _dstStage
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        VkBufferMemoryBarrier2 result = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
        result.srcAccessMask = _srcAccessMask;
        result.dstAccessMask = _dstAccessMask;
        result.srcStageMask = _srcStage;
        result.dstStageMask = _dstStage;
        result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        result.buffer = _buffer;
        result.offset = 0;
        result.size = VK_WHOLE_SIZE;

        return result;
    }


    void createImage(
        stl::vector<Image_vk>& _results
        , const stl::vector<ImageAliasInfo>& _infos
        , const ImgInitProps_vk& _initProps
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        if (_infos.empty())
        {
            return;
        }

        const VkPhysicalDeviceMemoryProperties& memProps = s_renderVK->m_memProps;
        const VkDevice device = s_renderVK->m_device;
        const VkPhysicalDevice pd = s_renderVK->m_physicalDevice;

        _results.clear();

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

        VkImageFormatProperties imgProps;
        vkGetPhysicalDeviceImageFormatProperties(pd, _initProps.format, _initProps.type, VK_IMAGE_TILING_OPTIMAL, _initProps.usage, createInfo.flags, &imgProps);

        // create for aliases
        for (uint32_t ii = 0; ii < num; ++ii)
        {
            Image_vk& img = results[ii];
            VK_CHECK(vkCreateImage(device, &createInfo, 0, &(img.image)));
        }

        VkImage baseImg = results[0].image;

        VkMemoryRequirements memoryReqs;
        vkGetImageMemoryRequirements(device, baseImg, &memoryReqs);

        uint32_t memoryTypeIdx = selectMemoryType(memProps, memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkDeviceMemory memory = allocVkMemory(device, memoryReqs.size, memoryTypeIdx);

        for (uint32_t ii = 0; ii < num; ++ii)
        {
            Image_vk& img = results[ii];
            VK_CHECK(vkBindImageMemory(device, img.image, memory, 0));
        }

        // setting for aliases
        for (uint32_t ii = 0; ii < num; ++ii)
        {
            Image_vk& img = results[ii];

            img.resId = _infos[ii].imgId;
            img.defaultView = createImageView(img.image, _initProps.format, 0, _initProps.numMips, _initProps.numLayers, _initProps.viewType); // ImageView bind to the image handle it self, should create a new one for alias
            img.memory = memory;

            img.width = _initProps.width;
            img.height = _initProps.height;

            img.aspectMask = _initProps.aspectMask;
            img.format = _initProps.format;
            img.viewType = _initProps.viewType;

            img.numMips = _initProps.numMips;
            img.numLayers = _initProps.numLayers;
        }

        _results = std::move(results);
    }


    Image_vk createImage(
        const ImageAliasInfo& _info
        , const ImgInitProps_vk& _initProps
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        stl::vector<Image_vk> results;
        createImage(results, {1, _info}, _initProps);
        assert(!results.empty());

        return results[0];
    }


    void destroyImage(
        const VkDevice _device
        , const stl::vector<Image_vk>& _images
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        assert(!_images.empty());

        // If a memory object is mapped at the time it is freed, it is implicitly unmapped.
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
        Image_vk baseImg = _images[0];
        freeVkMemory(_device, baseImg.memory);

        for (const Image_vk& img : _images)
        {
            vkDestroyImageView(_device, img.defaultView, nullptr);
            vkDestroyImage(_device, img.image, nullptr);
        }
    }

    VkImageView createImageView(
        VkImage _image
        , VkFormat _format
        , uint32_t _baseMipLevel
        , uint32_t _levelCount
        , uint32_t _layerCount
        , VkImageViewType _viewType /*= VK_IMAGE_VIEW_TYPE_2D */
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        const VkDevice device = s_renderVK->m_device;

        VkImageAspectFlags aspectMask = (_format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = _image;
        createInfo.viewType = _viewType;
        createInfo.format = _format;
        createInfo.subresourceRange.aspectMask = aspectMask;
        createInfo.subresourceRange.baseMipLevel = _baseMipLevel;
        createInfo.subresourceRange.levelCount = _levelCount;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = ((_viewType == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : _layerCount);

        VkImageView imageView = VK_NULL_HANDLE;
        VK_CHECK(vkCreateImageView(device, &createInfo, 0, &imageView));
        return imageView;
    }


    VkImageMemoryBarrier2 imageBarrier(
        VkImage _image
        , VkImageAspectFlags _aspectMask
        , VkAccessFlags2 _srcAccessMask
        , VkImageLayout _oldLayout
        , VkPipelineStageFlags2 _srcStage
        , VkAccessFlags2 _dstAccessMask
        ,  VkImageLayout _newLayout
        , VkPipelineStageFlags2 _dstStage
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        VkImageMemoryBarrier2 result = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    
        result.srcAccessMask = _srcAccessMask;
        result.dstAccessMask = _dstAccessMask;
        result.oldLayout = _oldLayout;
        result.newLayout = _newLayout;
        result.srcStageMask = _srcStage;
        result.dstStageMask = _dstStage;
        result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        result.image = _image;
        result.subresourceRange.aspectMask = _aspectMask;
        result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        return result;
    }


    void pipelineBarrier(
        VkCommandBuffer _cmdBuffer
        , VkDependencyFlags _flags
        , size_t _memBarrierCount
        , const VkMemoryBarrier2* _memBarriers
        , size_t _bufferBarrierCount
        , const VkBufferMemoryBarrier2* _bufferBarriers
        , size_t _imageBarrierCount
        , const VkImageMemoryBarrier2* _imageBarriers
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        VkDependencyInfo di = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR };
        di.dependencyFlags = _flags;

        di.memoryBarrierCount = (uint32_t)_memBarrierCount;
        di.pMemoryBarriers = _memBarriers;
        di.bufferMemoryBarrierCount = (uint32_t)_bufferBarrierCount;
        di.pBufferMemoryBarriers = _bufferBarriers;
        di.imageMemoryBarrierCount = (uint32_t)_imageBarrierCount;
        di.pImageMemoryBarriers = _imageBarriers;
    
        vkCmdPipelineBarrier2(_cmdBuffer, &di);
    }


    VkSampler createSampler(
        VkFilter _filter, VkSamplerMipmapMode _mipMode, VkSamplerAddressMode _addrMode, VkSamplerReductionMode _reductionMode /*= VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE */
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        const VkDevice device = s_renderVK->m_device;

        VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

        createInfo.magFilter = _filter;
        createInfo.minFilter = _filter;
        createInfo.mipmapMode = _mipMode;
        createInfo.addressModeU = _addrMode;
        createInfo.addressModeV = _addrMode;
        createInfo.addressModeW = _addrMode;
        createInfo.minLod = 0.f;
        createInfo.maxLod = 16.f; // required by occlusion culling, or tiny object will just culled since greater pyramid lod does not exist;

        VkSamplerReductionModeCreateInfoEXT createInfoReduction = { VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT };
        if (_reductionMode != VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE)
        {
            createInfoReduction.reductionMode = _reductionMode;
            createInfo.pNext = &createInfoReduction;
        }

        VkSampler sampler = 0;
        VK_CHECK(vkCreateSampler(device, &createInfo, 0, &sampler));

        return sampler;
    }

    VkMemoryBarrier2 memoryBarrier(
        VkAccessFlags2 _srcAccessMask
        , VkAccessFlags2 _dstAccessMask
        , VkPipelineStageFlags2 _srcStage
        , VkPipelineStageFlags2 _dstStage
    )
    {
        KG_ZoneScopedC(Color::light_coral);

        VkMemoryBarrier2 result = { VK_STRUCTURE_TYPE_MEMORY_BARRIER_2 };

        result.srcAccessMask = _srcAccessMask;
        result.dstAccessMask = _dstAccessMask;
        result.srcStageMask = _srcStage;
        result.dstStageMask = _dstStage;

        return result;
    }
}
} // namespace kage