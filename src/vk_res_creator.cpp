#include "common.h"
#include "macro.h"

#include "memory_operation.h"
#include "config.h"
#include "vkz.h"
#include "res_creator.h"
#include "vk_resource.h"
#include "vk_shaders.h"
#include "vk_res_creator.h"
#include "vkz_structs.h"


namespace vkz
{
    VkBufferUsageFlags getBufferUsageFlags(BufferUsageFlags _usageFlags)
    {
        VkBufferUsageFlags usage = 0;

        if (_usageFlags & BufferUsageFlagBits::storage)
        {
            usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if (_usageFlags & BufferUsageFlagBits::uniform)
        {
            usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if (_usageFlags & BufferUsageFlagBits::vertex)
        {
            usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if (_usageFlags & BufferUsageFlagBits::index)
        {
            usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if (_usageFlags & BufferUsageFlagBits::indirect)
        {
            usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }
        if (_usageFlags & BufferUsageFlagBits::transfer_src)
        {
            usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
        if (_usageFlags & BufferUsageFlagBits::transfer_dst)
        {
            usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        return usage;
    }

    VkImageUsageFlags getImageUsageFlags(ImageUsageFlags _usageFlags)
    {
        VkImageUsageFlags usage = 0;

        if (_usageFlags & ImageUsageFlagBits::storage)
        {
            usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (_usageFlags & ImageUsageFlagBits::color_attachment)
        {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (_usageFlags & ImageUsageFlagBits::depth_stencil_attachment)
        {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (_usageFlags & ImageUsageFlagBits::sampled)
        {
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (_usageFlags & ImageUsageFlagBits::transfer_src)
        {
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (_usageFlags & ImageUsageFlagBits::transfer_dst)
        {
            usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        return usage;
    }

    VkImageType getImageType(ImageType _type)
    {
        VkImageType type = VK_IMAGE_TYPE_2D;

        switch (_type)
        {
            case ImageType::type_1d:
                type = VK_IMAGE_TYPE_1D;
                break;
            case ImageType::type_2d:
                type = VK_IMAGE_TYPE_2D;
                break;
            case ImageType::type_3d:
                type = VK_IMAGE_TYPE_3D;
                break;
            default:
                type = VK_IMAGE_TYPE_2D;
                break;
        }

        return type;
    }

    VkImageViewType getImageViewType(ImageViewType _viewType)
    {
        VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;

        switch (_viewType)
        {
        case ImageViewType::type_1d:
            type = VK_IMAGE_VIEW_TYPE_1D;
            break;
        case ImageViewType::type_2d:
            type = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case ImageViewType::type_3d:
            type = VK_IMAGE_VIEW_TYPE_3D;
            break;
        case ImageViewType::type_cube:
            type = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case ImageViewType::type_1d_array:
            type = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            break;
        case ImageViewType::type_2d_array:
            type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;
        case ImageViewType::type_cube_array:
            type = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            break;
        default:
            type = VK_IMAGE_VIEW_TYPE_2D;
            break;
        }

        return type;
    };

    VkImageLayout getImageLayout(ImageLayout _layout)
    {
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

        switch (_layout)
        {
            case ImageLayout::undefined:
                layout = VK_IMAGE_LAYOUT_UNDEFINED;
                break;
            case ImageLayout::general:
                layout = VK_IMAGE_LAYOUT_GENERAL;
                break;
            case ImageLayout::color_attachment_optimal:
                layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                break;
            case ImageLayout::depth_stencil_attachment_optimal:
                layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                break;
            case ImageLayout::depth_stencil_read_only_optimal:
                layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                break;
            case ImageLayout::shader_read_only_optimal:
                layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                break;
            case ImageLayout::transfer_src_optimal:
                layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                break;
            case ImageLayout::transfer_dst_optimal:
                layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                break;
            case ImageLayout::preinitialized:
                layout = VK_IMAGE_LAYOUT_PREINITIALIZED;
                break;
            case ImageLayout::depth_read_only_stencil_attachment_optimal:
                layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
                break;
            case ImageLayout::depth_attachment_stencil_read_only_optimal:
                layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
                break;
            case ImageLayout::depth_attachment_optimal:
                layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                break;
            case ImageLayout::depth_read_only_optimal:
                layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
                break;
            case ImageLayout::stencil_attachment_optimal:
                layout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
                break;
            case ImageLayout::stencil_read_only_optimal:
                layout = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
                break;
            case ImageLayout::read_only_optimal:
                layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                break;
            case ImageLayout::attacment_optimal:
                layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                break;
            case ImageLayout::present_optimal:
                layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                break;
            default:
                layout = VK_IMAGE_LAYOUT_UNDEFINED;
                break;
        }

        return layout;
    };

    VkMemoryPropertyFlags getMemPropFlags(MemoryPropFlags _memFlags)
    {
        VkMemoryPropertyFlags flags = 0;

        if (_memFlags & MemoryPropFlagBits::device_local)
        {
            flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
        if (_memFlags & MemoryPropFlagBits::host_visible)
        {
            flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        }
        if (_memFlags & MemoryPropFlagBits::host_coherent)
        {
            flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }
        if (_memFlags & MemoryPropFlagBits::host_cached)
        {
            flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        }
        if (_memFlags & MemoryPropFlagBits::lazily_allocated)
        {
            flags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        }

        return flags;
    }

    VkFormat getFormat(ResourceFormat _format)
    {
        VkFormat format = VK_FORMAT_UNDEFINED;

        switch (_format)
        {
        case vkz::ResourceFormat::r8_sint:
            format = VK_FORMAT_R8_SINT;
            break;
        case vkz::ResourceFormat::r8_uint:
            format = VK_FORMAT_R8_UINT;
            break;
        case vkz::ResourceFormat::r16_uint:
            format = VK_FORMAT_R16_UINT;
            break;
        case vkz::ResourceFormat::r16_sint:
            format = VK_FORMAT_R16_SINT;
            break;
        case vkz::ResourceFormat::r16_snorm:
            format = VK_FORMAT_R16_SNORM;
            break;
        case vkz::ResourceFormat::r16_unorm:
            format = VK_FORMAT_R16_UNORM;       
            break;
        case vkz::ResourceFormat::r32_uint:
            format = VK_FORMAT_R32_UINT;
            break;
        case vkz::ResourceFormat::r32_sint:
            format = VK_FORMAT_R32_SINT;
            break;
        case vkz::ResourceFormat::r32_sfloat:
            format = VK_FORMAT_R32_SFLOAT;
            break;
        case vkz::ResourceFormat::b8g8r8a8_snorm:
            format = VK_FORMAT_B8G8R8A8_SNORM;
            break;
        case vkz::ResourceFormat::b8g8r8a8_unorm:
            format = VK_FORMAT_B8G8R8A8_UNORM;
            break;
        case vkz::ResourceFormat::b8g8r8a8_sint:
            format = VK_FORMAT_B8G8R8A8_SINT;
            break;
        case vkz::ResourceFormat::b8g8r8a8_uint:
            format = VK_FORMAT_B8G8R8A8_UINT;
            break;
        case vkz::ResourceFormat::r8g8b8a8_snorm:
            format = VK_FORMAT_R8G8B8A8_SNORM;
            break;
        case vkz::ResourceFormat::r8g8b8a8_unorm:
            format = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        case vkz::ResourceFormat::r8g8b8a8_sint:
            format = VK_FORMAT_R8G8B8A8_SINT;
            break;
        case vkz::ResourceFormat::r8g8b8a8_uint:
            format = VK_FORMAT_R8G8B8A8_UINT;
            break;
        case vkz::ResourceFormat::d16:
            format = VK_FORMAT_D16_UNORM;
            break;
        case vkz::ResourceFormat::d32:
            format = VK_FORMAT_D32_SFLOAT;
            break;
        default:
            format = VK_FORMAT_UNDEFINED;
            break;
        }

        return format;
    }


    void ResCreator_vk::init(MemoryReader& _reader)
    {

    }

    void ResCreator_vk::createPass(MemoryReader& _reader)
    {
        PassCreateInfo_vk info;
        read(&_reader, info);

        // create pass
        if ((uint16_t)PassType::Compute == info.type)
        {
            createComputePass(_reader, info);
        }
        if ((uint16_t)PassType::Graphics == info.type)
        {
            createGraphicsPass(_reader, info);
        }
    }

    void ResCreator_vk::createImage(MemoryReader& _reader)
    {
        ImageCreateInfo info;
        read(&_reader, info);
        // void createImage(Image& result, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProps, const VkFormat format, const ImgInitProps createInfo, ImageAliases = {}, const uint32_t aliasCount = 0 );
        ImageAliasInfo* resArr = new ImageAliasInfo[info.resNum];
        read(&_reader, resArr, sizeof(ImageAliasInfo) * info.resNum);

        // so the res handle should map to the real buffer array
        std::vector<ImageAliasInfo> infoList(resArr, resArr + info.resNum);

        ImgInitProps initPorps{};
        initPorps.level = info.mips;
        initPorps.width = info.width;
        initPorps.height = info.height;
        initPorps.depth = info.depth;
        initPorps.format = getFormat(info.format);
        initPorps.usage = getImageUsageFlags(info.usage);
        initPorps.type = getImageType(info.type);
        initPorps.layout = getImageLayout(info.layout);
        initPorps.viewType = getImageViewType( info.viewType);

        std::vector<Image_vk> images;
        vkz::createImage(images, infoList, m_device, m_memProps, initPorps);

        assert(images.size() == info.resNum);
        assert(m_imageIds.size() == m_images.size());

        // fill buffers and IDs
        m_imageIds.resize(m_imageIds.size() + info.resNum);
        m_images.resize(m_images.size() + info.resNum);
        for (int i = 0; i < info.resNum; ++i)
        {
            m_imageIds[m_imageIds.size() - info.resNum + i] = resArr[i].imgId;
            m_images[m_images.size() - info.resNum + i] = images[i];
        }
    }

    void ResCreator_vk::createBuffer(MemoryReader& _reader)
    {
        BufferCreateInfo info;
        read(&_reader, info);

        BufferAliasInfo* resArr = new BufferAliasInfo[info.resNum];
        read(&_reader, resArr, sizeof(BufferAliasInfo) * info.resNum);

        // so the res handle should map to the real buffer array
        std::vector<BufferAliasInfo> infoList(resArr, resArr + info.resNum);

        std::vector<Buffer_vk> buffers;
        vkz::createBuffer(buffers, infoList, m_memProps, m_device, getBufferUsageFlags(info.usage), getMemPropFlags(info.memFlags));

        assert(buffers.size() == info.resNum);
        assert(m_bufferIds.size() == m_buffers.size());

        // fill buffers and IDs
        m_bufferIds.resize(m_bufferIds.size() + info.resNum);
        m_buffers.resize(m_buffers.size() + info.resNum);
        for (int i = 0; i < info.resNum; ++i)
        {
            m_bufferIds[m_bufferIds.size() - info.resNum + i] = resArr[i].bufId;
            m_buffers[m_buffers.size() - info.resNum + i] = buffers[i];
        }

        VKZ_DELETE_ARRAY(resArr);
    }


    void ResCreator_vk::createComputePass(MemoryReader& _reader, const PassCreateInfo_vk& _info)
    {
        VkPipelineCache pipelineCache = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;


        //VkPipeline createComputePipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const Shader_vk & shader, Constants constants = {});
    }


    void ResCreator_vk::createGraphicsPass(MemoryReader& _reader, const PassCreateInfo_vk& _info)
    {

    }

} // namespace vkz