#pragma once

#include "kage_inner.h"
#include "kage_rhi_vk.h"

namespace kage { namespace vk
{
    struct Buffer_vk
    {
        uint16_t resId;

        VkBuffer buffer;
        VkDeviceMemory memory;
        void* data;
        size_t size;
        uint32_t fillVal;
    };

    Buffer_vk createBuffer(
        const BufferAliasInfo& _info
        , VkBufferUsageFlags _usage
        , VkMemoryPropertyFlags _memFlags
    );
    
    void createBuffer(
        stl::vector<Buffer_vk>& _results
        , const stl::vector<BufferAliasInfo> _infos
        , VkBufferUsageFlags _usage
        , VkMemoryPropertyFlags _memFlags
    );

    void flushBuffer(
        const Buffer_vk& _buffer
        , uint32_t _offset = 0
    );

    // destroy a list of buffers, which shares the same memory
    void destroyBuffer(
        const stl::vector<Buffer_vk>& _buffers
    );

    VkBufferMemoryBarrier2 bufferBarrier(
        VkBuffer buffer
        , VkAccessFlags2 srcAccessMask
        , VkPipelineStageFlags2 srcStage
        , VkAccessFlags2 dstAccessMask
        , VkPipelineStageFlags2 dstStage
    );

    struct Image_vk
    {
        uint32_t ID;
        uint16_t resId;

        VkImage image;
        VkImageView defaultView;
        VkDeviceMemory memory;
        VkImageAspectFlags  aspectMask;
        VkImageViewType     viewType;
        VkFormat            format;

        uint16_t numMips;
        uint16_t numLayers;
        uint32_t width, height;
    };

    struct ImgAliasInfo_vk
    {
        uint16_t resId;
    };

    struct ImgInitProps_vk
    {
        uint16_t numMips;
        uint16_t numLayers;

        uint32_t width;
        uint32_t height;
        uint32_t depth;

        VkFormat            format;
        VkImageUsageFlags   usage;
        VkImageType         type{ VK_IMAGE_TYPE_2D };
        VkImageLayout       layout{ VK_IMAGE_LAYOUT_GENERAL };
        VkImageViewType     viewType{ VK_IMAGE_VIEW_TYPE_2D };
        VkImageAspectFlags  aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT };
    };

    Image_vk createImage(
        const ImageAliasInfo& info
        , const ImgInitProps_vk& _initProps
    );
    
    void createImage(
        stl::vector<Image_vk>& _results
        , const stl::vector<ImageAliasInfo>& _infos
        , const ImgInitProps_vk& _initProps
    );
    
    // destroy a list of buffers, which shares the same memory
    void destroyImage(
        const VkDevice _device
        , const stl::vector<Image_vk>& _images
    );


    VkImageView createImageView(
        VkImage _image
        , VkFormat _format
        , uint32_t _baseMipLevel
        , uint32_t _levelCount
        , VkImageViewType _viewType = VK_IMAGE_VIEW_TYPE_2D
    );

    VkImageMemoryBarrier2 imageBarrier(
        VkImage                     _image
        , VkImageAspectFlags        _aspectMask
        , VkAccessFlags2            _srcAccessMask
        , VkImageLayout             _oldLayout
        , VkPipelineStageFlags2     _srcStage
        , VkAccessFlags2            _dstAccessMask
        , VkImageLayout             _newLayout
        , VkPipelineStageFlags2     _dstStage
    );

    void pipelineBarrier(
        VkCommandBuffer _cmdBuffer
        , VkDependencyFlags _flags
        , size_t _memBarrierCount
        , const VkMemoryBarrier2* _memBarriers
        , size_t _bufferBarrierCount
        , const VkBufferMemoryBarrier2* _bufferBarriers
        , size_t _imageBarrierCount
        , const VkImageMemoryBarrier2* _imageBarriers
    );


    VkSampler createSampler(
        VkFilter _filter
        , VkSamplerMipmapMode _mipMode
        , VkSamplerAddressMode _addrMode
        , VkSamplerReductionMode _reductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE
    );


    // global barrier
    VkMemoryBarrier2 memoryBarrier(
        VkAccessFlags2              _srcAccessMask
        , VkAccessFlags2            _dstAccessMask
        , VkPipelineStageFlags2     _srcStage
        , VkPipelineStageFlags2     _dstStage
    );
}
}