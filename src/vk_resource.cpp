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

    void createBuffer(VK_Buffer& result, const VkPhysicalDeviceMemoryProperties& memoryProps, VkDevice device, size_t sz
        , VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags, BufferList alisings /* = {}*/)
    {
        VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        createInfo.size = sz;
        createInfo.usage = usage;

        VkBuffer buffer = 0;
        VK_CHECK(vkCreateBuffer(device, &createInfo, 0, &buffer));

        for (VK_Buffer* const buf : alisings)
        {
            VK_CHECK(vkCreateBuffer(device, &createInfo, 0, &buf->buffer));
        }

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
        
            for (VK_Buffer* const buf : alisings)
            {
                VK_CHECK(vkMapMemory(device, memory, 0, sz, 0, &buf->data));
            }
        }

        for (VK_Buffer* const buf : alisings)
        {
            VK_CHECK(vkBindBufferMemory(device, buf->buffer, memory, 0));
            buf->memory = memory;
            buf->size = sz;
        }

        result.buffer = buffer;
        result.memory = memory;
        result.data = data;
        result.size = sz;

        // setting for aliases
        for (VK_Buffer* const buf : alisings)
        {
            buf->memory = memory;
            buf->data = data;
            buf->size = sz;
        }
    }

    void createBuffer(std::vector<VK_Buffer>& _results, const std::vector<VK_BufAliasInfo> _infos, const VkPhysicalDeviceMemoryProperties& _memProps
            , const VkDevice _device, const VkBufferUsageFlags _usage, const VkMemoryPropertyFlags _memFlags)
    {
        if (_infos.empty())
        {
            return;
        }

        assert(_results.size() == _infos.size());

        uint32_t size = (uint32_t)_results.size();
        std::vector<VK_Buffer> results(size);

        for (uint32_t ii = 0; ii < size; ++ii)
        {
            VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            createInfo.size = _infos[ii].size;
            createInfo.usage = _usage;

            VK_Buffer& buf = results[ii];

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
            VK_Buffer& buf = results[ii];
            VK_CHECK(vkBindBufferMemory(_device, buf.buffer, memory, 0));
            buf.memory = memory;
            buf.size = _infos[ii].size;
            buf.resId = _infos[ii].resId;
        }

        // map to local memory
        if (_memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            for (uint32_t ii = 0; ii < size; ++ii)
            {
                VK_Buffer& buf = results[ii];
                VK_CHECK(vkMapMemory(_device, memory, 0, buf.size, 0, &(buf.data)));
            }
        }

        _results = results;
    }

    void uploadBuffer(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const VK_Buffer& buffer, const VK_Buffer& scratch, const void* data, size_t size)
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
    void flushBuffer(VkDevice device, const VK_Buffer& buffer, uint32_t offset /* = 0*/)
    {
        VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        range.memory = buffer.memory;
        range.size = VK_WHOLE_SIZE;
        range.offset = offset;

        VK_CHECK(vkFlushMappedMemoryRanges(device, 1, &range));
    }

    void destroyBuffer(VkDevice device, const VK_Buffer& buffer, BufferList aliases /* = {} */)
    {
        // depends on this doc:
        //    If a memory object is mapped at the time it is freed, it is implicitly unmapped.
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
        vkFreeMemory(device, buffer.memory, 0);
        vkDestroyBuffer(device, buffer.buffer, 0);

        for (VK_Buffer* const buf : aliases)
        {
            // no need to free memory, because it is shared with the original one
            vkDestroyBuffer(device, buf->buffer, 0);
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



    void createImage(VK_Image& result, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProps
        , const VkFormat format, const ImgInitProps imgProps, ImageAliases aliases /*= {}*/, const uint32_t aliasCount /* = 0*/)
    {
        VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

        createInfo.imageType = imgProps.type;
        createInfo.format = format;
        createInfo.extent = { imgProps.width, imgProps.height, 1u };
        createInfo.mipLevels = imgProps.mipLevels;
        createInfo.arrayLayers = (imgProps.viewType == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.usage = imgProps.usage;
	    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (imgProps.viewType == VK_IMAGE_VIEW_TYPE_CUBE)
        {
            createInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        VkImage image = 0;
        VK_CHECK(vkCreateImage(device, &createInfo, 0, &image));

        // create for aliases
        for (VK_Image* const img : aliases)
        {
            VK_CHECK(vkCreateImage(device, &createInfo, 0, &img->image));
        }

        VkMemoryRequirements memoryReqs;
        vkGetImageMemoryRequirements(device, image, &memoryReqs);

        uint32_t memoryTypeIdx = selectMemoryType(memoryProps, memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memoryReqs.size;
        allocInfo.memoryTypeIndex = memoryTypeIdx;

        VkDeviceMemory memory = 0;
        VK_CHECK(vkAllocateMemory(device, &allocInfo, 0, &memory));


        VK_CHECK(vkBindImageMemory(device, image, memory, 0));
        for (VK_Image* const img : aliases) // bind memory for alias
        {
            VK_CHECK(vkBindImageMemory(device, img->image, memory, 0));
        }

        // calculate ID for render graph
        result.ID = hash32(image);
        result.image = image;
        result.imageView = createImageView(device, image, format, 0, imgProps.mipLevels, imgProps.viewType);
        result.memory = memory;

        result.width = imgProps.width;
        result.height = imgProps.height;
        result.mipLevels = imgProps.mipLevels;

        // setting for aliases
        for (VK_Image* const img : aliases)
        {
            img->ID = hash32(img->image);
            img->imageView = createImageView(device, img->image, format, 0, imgProps.mipLevels, imgProps.viewType); // ImageView bind to the image handle it self, should create a new one for alias
            img->memory = memory;

            img->width = imgProps.width;
            img->height = imgProps.height;
            img->mipLevels = imgProps.mipLevels;
        }
    }


    void loadKtxFromFile(ktxTexture** texture, const char* path)
    {
        assert(path);

        ktxResult result = KTX_SUCCESS;
        result = ktxTexture_CreateFromNamedFile(path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture);
        assert(result == KTX_SUCCESS);
    }

    void loadTexture2DFromFile(VK_Image& tex2d, const char* path, VkDevice device, VkCommandPool cmdPool, VkQueue queue, const VkPhysicalDeviceMemoryProperties& memoryProps, VkFormat format, VkImageUsageFlags usage, VkImageLayout layout)
    {
        ktxTexture* ktxTex{ nullptr };
        loadKtxFromFile(&ktxTex, path);
        assert(ktxTex);

        uint32_t width = ktxTex->baseWidth;
        uint32_t height = ktxTex->baseHeight;
        uint32_t mipLevels = ktxTex->numLevels;

        ImgInitProps imgProps = {};
        imgProps.width = width;
        imgProps.height = height;
        imgProps.mipLevels = mipLevels;
        imgProps.type = VK_IMAGE_TYPE_2D;
        imgProps.usage = usage;
        imgProps.layout = layout;
        imgProps.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

        VK_Image texture = {};
        createImage(texture, device, memoryProps, format, imgProps);

        ktx_uint8_t* ktxTexData = ktxTexture_GetData(ktxTex);
        ktx_size_t ktxTexDataSize = ktxTexture_GetDataSize(ktxTex);

        VK_Buffer scratch = {};
        createBuffer(scratch, memoryProps, device, ktxTexDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool = cmdPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer uploadCmd = 0;
        VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &uploadCmd));
        assert(uploadCmd);
        uploadImage(device, cmdPool, uploadCmd, queue, texture, scratch, ktxTexData, ktxTexDataSize, layout);

        ktxTexture_Destroy(ktxTex);
        destroyBuffer(device, scratch);

        VkSampler sampler = createSampler(device);
        assert(sampler);

        tex2d.width = width;
        tex2d.height = height;
        tex2d.mipLevels = mipLevels;
    
        tex2d.image = texture.image;
        tex2d.imageView = texture.imageView;
    }


    void loadTextureCubeFromFile(VK_Image& texCube, const char* path, VkDevice device, VkCommandPool cmdPool, VkQueue queue, const VkPhysicalDeviceMemoryProperties& memoryProps, VkFormat format, VkImageUsageFlags usage, VkImageLayout layout)
    {
        ktxTexture* ktxTex{ nullptr };
        loadKtxFromFile(&ktxTex, path);
        assert(ktxTex && ktxTex->isCubemap);

        uint32_t width = ktxTex->baseWidth;
        uint32_t height = ktxTex->baseHeight;
        uint32_t mipLevels = ktxTex->numLevels;

        ImgInitProps imgProps = {};
        imgProps.width = width;
        imgProps.height = height;
        imgProps.mipLevels = mipLevels;
        imgProps.usage = usage;
        imgProps.layout = layout;
        imgProps.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

        VK_Image texture = {};
        createImage(texture, device, memoryProps, format, imgProps);

        ktx_uint8_t* ktxTexData = ktxTexture_GetData(ktxTex);
        ktx_size_t ktxTexDataSize = ktxTexture_GetDataSize(ktxTex);

        VK_Buffer scratch = {};
        createBuffer(scratch, memoryProps, device, ktxTexDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool = cmdPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer uploadCmd = 0;
        VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &uploadCmd));
        assert(uploadCmd);

        memcpy(scratch.data, ktxTexData, ktxTexDataSize);

        std::vector<VkBufferImageCopy> regions;
        for (uint32_t face = 0; face < 6; ++face)
        {
            for (uint32_t level = 0; level < mipLevels; ++level)
            {
                ktx_size_t offset = 0;
                KTX_error_code result = ktxTexture_GetImageOffset(ktxTex, level, 0, face, &offset);
                assert(result == KTX_SUCCESS);

                VkBufferImageCopy region = {};
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.layerCount = 1;
                region.imageSubresource.baseArrayLayer = face;
                region.imageSubresource.mipLevel = level;
                region.imageExtent.width = texture.width >> level;
                region.imageExtent.height = texture.height >> level;
                region.imageExtent.depth = 1;
                region.bufferOffset = offset;

                regions.push_back(region);
            }
        }

        VK_CHECK(vkResetCommandPool(device, cmdPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(uploadCmd, &beginInfo));

        VkImageMemoryBarrier copyBarrier[1] = {};
        copyBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        copyBarrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        copyBarrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        copyBarrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copyBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copyBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copyBarrier[0].image = texture.image;
        copyBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyBarrier[0].subresourceRange.levelCount = mipLevels;
        copyBarrier[0].subresourceRange.layerCount = 6;

        vkCmdPipelineBarrier(uploadCmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, copyBarrier);

        vkCmdCopyBufferToImage(uploadCmd, scratch.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)regions.size(), regions.data());

        VkImageMemoryBarrier useBarrier[1] = {};
        useBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        useBarrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        useBarrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        useBarrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        useBarrier[0].newLayout = layout;
        useBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        useBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        useBarrier[0].image = texture.image;
        useBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        useBarrier[0].subresourceRange.levelCount = mipLevels;
        useBarrier[0].subresourceRange.layerCount = 6;
        vkCmdPipelineBarrier(uploadCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, useBarrier);

        VK_CHECK(vkEndCommandBuffer(uploadCmd));

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &uploadCmd;

        VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

        VK_CHECK(vkDeviceWaitIdle(device));

        ktxTexture_Destroy(ktxTex);
        destroyBuffer(device, scratch);

        texCube.width = width;
        texCube.height = height;
        texCube.mipLevels = 1;

        texCube.image = texture.image;
        texCube.imageView = texture.imageView;
        texCube.memory = texture.memory;
    }

    void uploadImage(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const VK_Image& image, const VK_Buffer& scratch, const void* data, size_t size, VkImageLayout layout, const uint32_t regionCount /*= 1*/, uint32_t mipLevels /*= 1*/)
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

    void destroyImage(VkDevice device, const VK_Image& image, ImageAliases aliases /*= {}*/)
    {
        vkFreeMemory(device, image.memory, 0);
        vkDestroyImageView(device, image.imageView, 0);
        vkDestroyImage(device, image.image, 0);

        for (VK_Image* const img : aliases)
        {
            // no need to free memory, because it is shared with the original one
            vkDestroyImageView(device, img->imageView, 0);
            vkDestroyImage(device, img->image, 0);
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