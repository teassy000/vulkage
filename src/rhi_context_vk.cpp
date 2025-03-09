#include "common.h"
#include "macro.h"

#include "config.h"
#include "util.h"
#include "kage_math.h"
#include "rhi_context_vk.h"

#include <algorithm> //sort
#include "bx/hash.h"
#include "command_buffer.h"

#include "FidelityFX/host/backends/vk/ffx_vk.h"
#include "FidelityFX/host/ffx_interface.h"

namespace kage { namespace vk
{
    RHIContext_vk* s_renderVK = nullptr;


#define VK_DESTROY_FUNC(_name)                                                          \
    void vkDestroy(Vk##_name& _obj)                                                     \
    {                                                                                   \
        KG_ZoneScopedC(Color::indian_red);                                             \
        if (VK_NULL_HANDLE != _obj)                                                     \
        {                                                                               \
            vkDestroy##_name(s_renderVK->m_device, _obj.vk, s_renderVK->m_allocatorCb); \
            _obj = VK_NULL_HANDLE;                                                      \
        }                                                                               \
    }                                                                                   \
    void release(Vk##_name& _obj)                                                       \
    {                                                                                   \
        s_renderVK->release(_obj);                                                      \
    }

    VK_DESTROY

#undef VK_DESTROY_FUNC

    void vkDestroy(VkSurfaceKHR& _obj)
    {
        KG_ZoneScopedC(Color::indian_red);
        if (VK_NULL_HANDLE != _obj)
        {
            vkDestroySurfaceKHR(s_renderVK->m_instance, _obj.vk, s_renderVK->m_allocatorCb);
            _obj = VK_NULL_HANDLE;
        }
    }

    void vkDestroy(VkDeviceMemory& _obj)
    {
        KG_ZoneScopedC(Color::indian_red);
        if (VK_NULL_HANDLE != _obj)
        {
            vkFreeMemory(s_renderVK->m_device, _obj.vk, s_renderVK->m_allocatorCb);
            KG_ProfFree((void*)_obj.vk);
            _obj = VK_NULL_HANDLE;
        }
    }

    void vkDestroy(VkDescriptorSet& _obj)
    {
        KG_ZoneScopedC(Color::indian_red);
        if (VK_NULL_HANDLE != _obj)
        {
            vkFreeDescriptorSets(s_renderVK->m_device, s_renderVK->getDescriptorPool(_obj), 1, &_obj);
            _obj = VK_NULL_HANDLE;
        }
    }

    void release(VkSurfaceKHR& _obj)
    {
        s_renderVK->release(_obj);
    }

    void release(VkDeviceMemory& _obj)
    {
        s_renderVK->release(_obj);
    }

    void release(VkDescriptorSet& _obj)
    {
        s_renderVK->release(_obj);
    }


    struct DebugNames
    {
        using HashKey = uint32_t;
        using NameMap = stl::unordered_map<HashKey, String>;

        ~DebugNames()
        {
            m_names.clear();
        }

        template<typename Ty>
        void setName(Ty _obj, const char* _name)
        {
            VkObjectType type = getType<Ty>();
            uint64_t handle = (uint64_t)(_obj.vk);

            bx::HashMurmur2A hash;
            hash.begin();
            hash.add(type);
            hash.add(handle);

            uint32_t hashKey = hash.end();
            String str(_name);
            m_names.insert({ hashKey, str });
        }

        template<typename Ty>
        void unsetName(Ty _obj)
        {
            VkObjectType type = getType<Ty>();
            uint64_t handle = (uint64_t)(_obj.vk);

            bx::HashMurmur2A hash;
            hash.begin();
            hash.add(type);
            hash.add(handle);

            uint32_t hashKey = hash.end();
            
            NameMap::iterator it = m_names.find(hashKey);
            if (m_names.end() != it)
            {
                m_names.erase(it);
            }
        }


        template<typename Ty>
        const char* get(Ty _obj)
        {
            VkObjectType type = getType<Ty>();
            uint64_t handle = (uint64_t)(_obj.vk);

            bx::HashMurmur2A hash;
            hash.begin();
            hash.add(type);
            hash.add(handle);

            uint32_t hashKey = hash.end();

            NameMap::iterator it = m_names.find(hashKey);
            if (m_names.end() != it)
            {
                return it->second.getCPtr();
            }

            return "";
        }

        NameMap m_names;
    };

    static DebugNames* s_debugNames{nullptr};

    DebugNames* getDebugNames()
    {
        if (nullptr == s_debugNames)
        {
            s_debugNames = BX_NEW(g_bxAllocator, DebugNames);
        }

        return s_debugNames;
    }


    template<typename Ty>
    void setLocalDebugName(Ty _obj, const char* _name)
    {
        getDebugNames()->setName(_obj, _name);
    }

    template<typename Ty>
    void unsetLocalDebugName(Ty _obj)
    {
        getDebugNames()->unsetName(_obj);
    }

    template<typename Ty>
    const char* getLocalDebugName(Ty _obj)
    {
        return getDebugNames()->get(_obj);
    }

    void shutdownLocalDebugName()
    {
        bx::deleteObject(g_bxAllocator, s_debugNames);
        s_debugNames = nullptr;
    }

    RHIContext* rendererCreate(const Resolution& _resolution, void* _wnd)
    {
        s_renderVK = BX_NEW(g_bxAllocator, RHIContext_vk)(g_bxAllocator);

        s_renderVK->init(_resolution, _wnd);
        
        return s_renderVK;
    }

    void rendererDestroy()
    {
        shutdownLocalDebugName();

        s_renderVK->shutdown();
        bx::deleteObject(g_bxAllocator, s_renderVK);
        s_renderVK = nullptr;
    }

    const char* getExtName(VulkanSupportExtension _ext)
    {
        switch (_ext)
        {
        case kage::VulkanSupportExtension::ext_swapchain:
            return VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        case kage::VulkanSupportExtension::ext_push_descriptor:
            return VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME;
        case kage::VulkanSupportExtension::ext_8bit_storage:
            return VK_KHR_8BIT_STORAGE_EXTENSION_NAME;
        case kage::VulkanSupportExtension::ext_16bit_storage:
            return VK_KHR_16BIT_STORAGE_EXTENSION_NAME;
        case kage::VulkanSupportExtension::ext_mesh_shader:
            return VK_EXT_MESH_SHADER_EXTENSION_NAME;
        case kage::VulkanSupportExtension::ext_spirv_1_4:
            return VK_KHR_SPIRV_1_4_EXTENSION_NAME;
        case kage::VulkanSupportExtension::ext_shader_float_controls:
            return VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME;
        case kage::VulkanSupportExtension::ext_shader_draw_parameters:
            return VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME;
        case kage::VulkanSupportExtension::ext_draw_indriect_count:
            return VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME;
        case kage::VulkanSupportExtension::ext_shader_float16_int8:
            return VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME;
        case kage::VulkanSupportExtension::ext_fragment_shading_rate:
            return VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME;
        default:
            return nullptr;
        }
    }

    VkIndexType getIndexType(IndexType _type)
    {
        switch (_type)
        {
        case kage::IndexType::uint16:
            return VK_INDEX_TYPE_UINT16;
        case kage::IndexType::uint32:
            return VK_INDEX_TYPE_UINT32;
        default:
            return VK_INDEX_TYPE_UINT16;
        }
    }

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
        if (_usageFlags & BufferUsageFlagBits::uniform_texel)
        {
            usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        }
        if (_usageFlags & BufferUsageFlagBits::storage_texel)
        {
            usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
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

    const VkImageAspectFlags getImageAspectFlags(ImageAspectFlags _aspectFlags)
    {
        VkImageAspectFlags flags = 0;

        if (_aspectFlags & ImageAspectFlagBits::color)
        {
            flags |= VK_IMAGE_ASPECT_COLOR_BIT;
        }
        if (_aspectFlags & ImageAspectFlagBits::depth)
        {
            flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (_aspectFlags & ImageAspectFlagBits::stencil)
        {
            flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        return flags;
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

    VkFilter getFilter(SamplerFilter _filter)
    {
        VkFilter filter = VK_FILTER_NEAREST;
        switch (_filter)
        {
        case SamplerFilter::nearest:
            filter = VK_FILTER_NEAREST;
            break;
        case SamplerFilter::linear:
            filter = VK_FILTER_LINEAR;
            break;
        default:
            filter = VK_FILTER_NEAREST;
            break;
        }
        return filter;
    }

    VkSamplerMipmapMode getSamplerMipmapMode(SamplerMipmapMode _mode)
    {
        VkSamplerMipmapMode mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        switch (_mode)
        {
        case SamplerMipmapMode::nearest:
            mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case SamplerMipmapMode::linear:
            mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        default:
            mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        }
        return mode;
    }

    VkSamplerAddressMode getSamplerAddressMode(SamplerAddressMode _mode)
    {
        VkSamplerAddressMode mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        switch (_mode)
        {
        case SamplerAddressMode::repeat:
            mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        case SamplerAddressMode::mirrored_repeat:
            mode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        case SamplerAddressMode::clamp_to_edge:
            mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case SamplerAddressMode::clamp_to_border:
            mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            break;
        case SamplerAddressMode::mirror_clamp_to_edge:
            mode = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
            break;
        default:
            mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        }
        return mode;
    }

    VkSamplerReductionMode getSamplerReductionMode(SamplerReductionMode _reduction)
    {
        VkSamplerReductionMode reduction = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
        switch (_reduction)
        {
            case SamplerReductionMode::weighted_average:
                reduction = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
                break;
            case SamplerReductionMode::min:
                reduction = VK_SAMPLER_REDUCTION_MODE_MIN;
                break;
            case SamplerReductionMode::max:
                reduction = VK_SAMPLER_REDUCTION_MODE_MAX;
                break;
            default:
                reduction = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
                break;
        }

        return reduction;
    }

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

    VkFormat getFormat(ResourceFormat _fmt)
    {
        VkFormat format = VK_FORMAT_UNDEFINED;

        // convert all resource format to vulkan format
        switch (_fmt)
        {
        case kage::ResourceFormat::bc1:                     format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK; break;
        case kage::ResourceFormat::bc3:                     format = VK_FORMAT_BC3_UNORM_BLOCK;     break;
        case kage::ResourceFormat::bc5:                     format = VK_FORMAT_BC5_UNORM_BLOCK;     break;
        case kage::ResourceFormat::r8_snorm:                format = VK_FORMAT_R8_SNORM;            break;
        case kage::ResourceFormat::r8_unorm:                format = VK_FORMAT_R8_UNORM;            break;
        case kage::ResourceFormat::r8_sint:                 format = VK_FORMAT_R8_SINT;             break;
        case kage::ResourceFormat::r8_uint:                 format = VK_FORMAT_R8_UINT;             break;
        case kage::ResourceFormat::r8g8_snorm:              format = VK_FORMAT_R8G8_SNORM;          break;
        case kage::ResourceFormat::r8g8_unorm:              format = VK_FORMAT_R8G8_UNORM;          break;
        case kage::ResourceFormat::r8g8_sint:               format = VK_FORMAT_R8G8_SINT;           break;
        case kage::ResourceFormat::r8g8_uint:               format = VK_FORMAT_R8G8_UINT;           break;
        case kage::ResourceFormat::r8g8b8_snorm:            format = VK_FORMAT_R8G8B8_SNORM;        break;
        case kage::ResourceFormat::r8g8b8_unorm:            format = VK_FORMAT_R8G8B8_UNORM;        break;
        case kage::ResourceFormat::r8g8b8_sint:             format = VK_FORMAT_R8G8B8_SINT;         break;
        case kage::ResourceFormat::r8g8b8_uint:             format = VK_FORMAT_R8G8B8_UINT;         break;
        case kage::ResourceFormat::b8g8r8_snorm:            format = VK_FORMAT_B8G8R8_SNORM;        break;
        case kage::ResourceFormat::b8g8r8_unorm:            format = VK_FORMAT_B8G8R8_UNORM;        break;
        case kage::ResourceFormat::b8g8r8_sint:             format = VK_FORMAT_B8G8R8_SINT;         break;
        case kage::ResourceFormat::b8g8r8_uint:             format = VK_FORMAT_B8G8R8_UINT;         break;
        case kage::ResourceFormat::r8g8b8a8_snorm:          format = VK_FORMAT_R8G8B8A8_SNORM;      break;
        case kage::ResourceFormat::r8g8b8a8_unorm:          format = VK_FORMAT_R8G8B8A8_UNORM;      break;
        case kage::ResourceFormat::r8g8b8a8_sint:           format = VK_FORMAT_R8G8B8A8_SINT;       break;
        case kage::ResourceFormat::r8g8b8a8_uint:           format = VK_FORMAT_R8G8B8A8_UINT;       break;
        case kage::ResourceFormat::b8g8r8a8_snorm:          format = VK_FORMAT_B8G8R8A8_SNORM;      break;
        case kage::ResourceFormat::b8g8r8a8_unorm:          format = VK_FORMAT_B8G8R8A8_UNORM;      break;
        case kage::ResourceFormat::b8g8r8a8_sint:           format = VK_FORMAT_B8G8R8A8_SINT;       break;
        case kage::ResourceFormat::b8g8r8a8_uint:           format = VK_FORMAT_B8G8R8A8_UINT;       break;
        case kage::ResourceFormat::r16_uint:                format = VK_FORMAT_R16_UINT;            break;
        case kage::ResourceFormat::r16_sint:                format = VK_FORMAT_R16_SINT;            break;
        case kage::ResourceFormat::r16_snorm:               format = VK_FORMAT_R16_SNORM;           break;
        case kage::ResourceFormat::r16_unorm:               format = VK_FORMAT_R16_UNORM;           break;
        case kage::ResourceFormat::r16_sfloat:              format = VK_FORMAT_R16_SFLOAT;          break;
        case kage::ResourceFormat::r16g16_unorm:            format = VK_FORMAT_R16G16_UNORM;        break;
        case kage::ResourceFormat::r16g16_snorm:            format = VK_FORMAT_R16G16_SNORM;        break;
        case kage::ResourceFormat::r16g16_uint:             format = VK_FORMAT_R16G16_UINT;         break;
        case kage::ResourceFormat::r16g16_sint:             format = VK_FORMAT_R16G16_SINT;         break;
        case kage::ResourceFormat::r16g16_sfloat:           format = VK_FORMAT_R16G16_SFLOAT;       break;
        case kage::ResourceFormat::r16g16b16a16_unorm:      format = VK_FORMAT_R16G16B16A16_UNORM;  break;
        case kage::ResourceFormat::r16g16b16a16_snorm:      format = VK_FORMAT_R16G16B16A16_SNORM;  break;
        case kage::ResourceFormat::r16g16b16a16_uint:       format = VK_FORMAT_R16G16B16A16_UINT;   break;
        case kage::ResourceFormat::r16g16b16a16_sint:       format = VK_FORMAT_R16G16B16A16_SINT;   break;
        case kage::ResourceFormat::r16g16b16a16_sfloat:     format = VK_FORMAT_R16G16B16A16_SFLOAT; break;
        case kage::ResourceFormat::r32_uint:                format = VK_FORMAT_R32_UINT;            break;
        case kage::ResourceFormat::r32_sint:                format = VK_FORMAT_R32_SINT;            break;
        case kage::ResourceFormat::r32_sfloat:              format = VK_FORMAT_R32_SFLOAT;          break;
        case kage::ResourceFormat::r32g32_uint:             format = VK_FORMAT_R32G32_UINT;         break;
        case kage::ResourceFormat::r32g32_sint:             format = VK_FORMAT_R32G32_SINT;         break;
        case kage::ResourceFormat::r32g32_sfloat:           format = VK_FORMAT_R32G32_SFLOAT;       break;
        case kage::ResourceFormat::r32g32b32_uint:          format = VK_FORMAT_R32G32B32_UINT;      break;
        case kage::ResourceFormat::r32g32b32_sint:          format = VK_FORMAT_R32G32B32_SINT;      break;
        case kage::ResourceFormat::r32g32b32_sfloat:        format = VK_FORMAT_R32G32B32_SFLOAT;    break;
        case kage::ResourceFormat::r32g32b32a32_uint:       format = VK_FORMAT_R32G32B32A32_UINT;   break;
        case kage::ResourceFormat::r32g32b32a32_sint:       format = VK_FORMAT_R32G32B32A32_SINT;   break;
        case kage::ResourceFormat::r32g32b32a32_sfloat:     format = VK_FORMAT_R32G32B32A32_SFLOAT;     break;
        case kage::ResourceFormat::b5g6r5_unorm:            format = VK_FORMAT_B5G6R5_UNORM_PACK16;     break;
        case kage::ResourceFormat::r5g6b5_unorm:            format = VK_FORMAT_R5G6B5_UNORM_PACK16;     break;
        case kage::ResourceFormat::b4g4r4a4_unorm:          format = VK_FORMAT_B4G4R4A4_UNORM_PACK16;   break;
        case kage::ResourceFormat::r4g4b4a4_unorm:          format = VK_FORMAT_R4G4B4A4_UNORM_PACK16;   break;
        case kage::ResourceFormat::b5g5r5a1_unorm:          format = VK_FORMAT_B5G5R5A1_UNORM_PACK16;   break;
        case kage::ResourceFormat::r5g5b5a1_unorm:          format = VK_FORMAT_R5G5B5A1_UNORM_PACK16;   break;
        case kage::ResourceFormat::a2r10g10b10_unorm:       format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;break;
        case kage::ResourceFormat::a2r10g10b10_uint:        format = VK_FORMAT_A2R10G10B10_UINT_PACK32; break;
        case kage::ResourceFormat::b10g11r11_sfloat:        format = VK_FORMAT_B10G11R11_UFLOAT_PACK32; break;
        case kage::ResourceFormat::d16:                     format = VK_FORMAT_D16_UNORM;               break;
        case kage::ResourceFormat::d24_sfloat_s8_uint:      format = VK_FORMAT_D24_UNORM_S8_UINT;       break;
        case kage::ResourceFormat::d32_sfloat:              format = VK_FORMAT_D32_SFLOAT;              break;
        default:
            format = VK_FORMAT_UNDEFINED; 
            break;
        }

        return format;
    }

    VkFormat getValidFormat(ResourceFormat _format, VkFormat _color, VkFormat _depth)
    {
        // undefined
        if (_format == ResourceFormat::undefined)
        {
            message(warning, "undefined color format! using attachment default format instead!");
            return _color;
        }
        if(_format == ResourceFormat::unknown_depth)
        {
            message(warning, "undefined depth format! using attachment default format instead!");
            return _depth;
        }

        return getFormat(_format);
    }

    VkPipelineBindPoint getBindPoint(const stl::vector<Shader_vk>& shaders)
    {
        VkShaderStageFlags stages = 0;
        for (const Shader_vk& shader : shaders)
        {
            stages |= shader.stage;
        }

        const uint32_t raytracingFlag = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR
            | VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR
            | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
            | VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR;

        if (stages & VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT)
        {
            return VK_PIPELINE_BIND_POINT_COMPUTE;
        }
        if (stages & raytracingFlag)
        {
            return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
        }

        return VK_PIPELINE_BIND_POINT_GRAPHICS;
    }

    VkCompareOp getCompareOp(CompareOp op)
    {
        VkCompareOp compareOp = VK_COMPARE_OP_NEVER;

        switch (op)
        {
        case CompareOp::never:
            compareOp = VK_COMPARE_OP_NEVER;
            break;
        case CompareOp::less:
            compareOp = VK_COMPARE_OP_LESS;
            break;
        case CompareOp::equal:
            compareOp = VK_COMPARE_OP_EQUAL;
            break;
        case CompareOp::less_or_equal:
            compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            break;
        case CompareOp::greater:
            compareOp = VK_COMPARE_OP_GREATER;
            break;
        case CompareOp::not_equal:
            compareOp = VK_COMPARE_OP_NOT_EQUAL;
            break;
        case CompareOp::greater_or_equal:
            compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
            break;
        case CompareOp::always:
            compareOp = VK_COMPARE_OP_ALWAYS;
            break;
        default:
            compareOp = VK_COMPARE_OP_NEVER;
            break;
        }

        return compareOp;
    }

    VkCullModeFlags getCullMode(CullModeFlags _mode)
    {
        VkCullModeFlags mode = VK_CULL_MODE_NONE;
        
        if (_mode & CullModeFlagBits::front)
        {
            mode |= VK_CULL_MODE_FRONT_BIT;
        }
        if (_mode & CullModeFlagBits::back)
        {
            mode |= VK_CULL_MODE_BACK_BIT;
        }

        return mode;
    }

    VkPolygonMode getPolygonMode(PolygonMode _mode)
    {
        VkPolygonMode mode = VK_POLYGON_MODE_FILL;
        switch (_mode)
        {
        case PolygonMode::fill:
            mode = VK_POLYGON_MODE_FILL;
            break;
        case PolygonMode::line:
            mode = VK_POLYGON_MODE_LINE;
            break;
        case PolygonMode::point:
            mode = VK_POLYGON_MODE_POINT;
            break;
        default:
            mode = VK_POLYGON_MODE_FILL;
            break;
        }
        return mode;
    }

    VkVertexInputRate getInputRate(VertexInputRate _rate)
    {
        VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX;

        switch (_rate)
        {
        case VertexInputRate::vertex:
            rate = VK_VERTEX_INPUT_RATE_VERTEX;
            break;
        case VertexInputRate::instance:
            rate = VK_VERTEX_INPUT_RATE_INSTANCE;
            break;
        default:
            rate = VK_VERTEX_INPUT_RATE_VERTEX;
            break;
        }

        return rate;
    }

    VkAccessFlags getAccessFlags(AccessFlags _flags)
    {
        VkAccessFlags flags = 0;

        if (_flags & AccessFlagBits::indirect_command_read)
        {
            flags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        }
        if (_flags & AccessFlagBits::index_read)
        {
            flags |= VK_ACCESS_INDEX_READ_BIT;
        }
        if (_flags & AccessFlagBits::vertex_attribute_read)
        {
            flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        }
        if (_flags & AccessFlagBits::uniform_read)
        {
            flags |= VK_ACCESS_UNIFORM_READ_BIT;
        }
        if (_flags & AccessFlagBits::input_attachment_read)
        {
            flags |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        }
        if (_flags & AccessFlagBits::shader_read)
        {
            flags |= VK_ACCESS_SHADER_READ_BIT;
        }
        if (_flags & AccessFlagBits::shader_write)
        {
            flags |= VK_ACCESS_SHADER_WRITE_BIT;
        }
        if (_flags & AccessFlagBits::color_attachment_read)
        {
            flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        }
        if (_flags & AccessFlagBits::color_attachment_write)
        {
            flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }
        if (_flags & AccessFlagBits::depth_stencil_attachment_read)
        {
            flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        }
        if (_flags & AccessFlagBits::depth_stencil_attachment_write)
        {
            flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }
        if (_flags & AccessFlagBits::transfer_read)
        {
            flags |= VK_ACCESS_TRANSFER_READ_BIT;
        }
        if (_flags & AccessFlagBits::transfer_write)
        {
            flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        if (_flags & AccessFlagBits::host_read)
        {
            flags |= VK_ACCESS_HOST_READ_BIT;
        }
        if (_flags & AccessFlagBits::host_write)
        {
            flags |= VK_ACCESS_HOST_WRITE_BIT;
        }
        if (_flags & AccessFlagBits::memory_read)
        {
            flags |= VK_ACCESS_MEMORY_READ_BIT;
        }
        if (_flags & AccessFlagBits::memory_write)
        {
            flags |= VK_ACCESS_MEMORY_WRITE_BIT;
        }
        if (_flags & AccessFlagBits::transform_feedback_write)
        {
            flags |= VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT;
        }
        if (_flags & AccessFlagBits::transform_feedback_counter_read)
        {
            flags |= VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT;
        }
        if (_flags & AccessFlagBits::transform_feedback_counter_write)
        {
            flags |= VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT;
        }
        if (_flags & AccessFlagBits::conditional_rendering_read)
        {
            flags |= VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
        }
        if (_flags & AccessFlagBits::color_attachment_read_noncoherent)
        {
            flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT;
        }
        if (_flags & AccessFlagBits::acceleration_structure_read)
        {
            flags |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        }
        if (_flags & AccessFlagBits::acceleration_structure_write)
        {
            flags |= VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        }
        if (_flags & AccessFlagBits::shading_rate_image_read)
        {
            flags |= VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV;
        }

        return flags;
    }

    VkAccessFlags getBindingAccess(BindingAccess _access)
    {
        VkAccessFlags flags = 0;

        switch (_access)
        {
        case kage::BindingAccess::read:
            flags = VK_ACCESS_SHADER_READ_BIT;
            break;
        case kage::BindingAccess::write:
            flags = VK_ACCESS_SHADER_WRITE_BIT;
            break;
        case kage::BindingAccess::read_write:
            flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            break;
        default:
            message(error, "invalid binding access enum! _access : %d", _access);
            break;
        }

        if (BindingAccess::read == _access)
        {
            flags = VK_ACCESS_SHADER_READ_BIT;
        }

        return flags;
        
    }

    VkImageLayout getBindingLayout(BindingAccess _access)
    {
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

        switch (_access)
        {
            case BindingAccess::read:
                layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                break;
            case BindingAccess::write:
                layout = VK_IMAGE_LAYOUT_GENERAL;
                break;
            case BindingAccess::read_write:
                layout = VK_IMAGE_LAYOUT_GENERAL;
                break;
            default:
                break;
        }

        return layout;
    }

    VkPipelineStageFlags getPipelineStageFlags(PipelineStageFlags _flags)
    {
        VkPipelineStageFlags flags = 0;

        if (_flags & PipelineStageFlagBits::top_of_pipe)
        {
            flags |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        if (_flags & PipelineStageFlagBits::indirect)
        {
            // why vk dose not have a dispatch indirect bit?
            // https://github.com/KhronosGroup/Vulkan-Docs/issues/176
            // the draw indirect will serve draw*/dispatch*/tracerays* commands
            flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        }
        if (_flags & PipelineStageFlagBits::vertex_input)
        {
            flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        }
        if (_flags & PipelineStageFlagBits::vertex_shader)
        {
            flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        }
        if (_flags & PipelineStageFlagBits::geometry_shader)
        {
            flags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        }
        if (_flags & PipelineStageFlagBits::fragment_shader)
        {
            flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        if (_flags & PipelineStageFlagBits::early_fragment_tests)
        {
            flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        if (_flags & PipelineStageFlagBits::late_fragment_tests)
        {
            flags |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        }
        if (_flags & PipelineStageFlagBits::color_attachment_output)
        {
            flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
        if (_flags & PipelineStageFlagBits::compute_shader)
        {
            flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        }
        if (_flags & PipelineStageFlagBits::transfer)
        {
            flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        if (_flags & PipelineStageFlagBits::bottom_of_pipe)
        {
            flags |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
        if (_flags & PipelineStageFlagBits::host)
        {
            flags |= VK_PIPELINE_STAGE_HOST_BIT;
        }
        if (_flags & PipelineStageFlagBits::all_graphics)
        {
            flags |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        }
        if (_flags & PipelineStageFlagBits::all_commands)
        {
            flags |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }
        if (_flags & PipelineStageFlagBits::task_shader)
        {
            flags |= VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT;
        }
        if (_flags & PipelineStageFlagBits::mesh_shader)
        {
            flags |= VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT;
        }
        return flags;
    }

    VkAttachmentLoadOp getAttachmentLoadOp(AttachmentLoadOp _op)
    {
        VkAttachmentLoadOp op = VK_ATTACHMENT_LOAD_OP_LOAD;

        switch (_op)
        {
        case AttachmentLoadOp::load:
            op = VK_ATTACHMENT_LOAD_OP_LOAD;
            break;
        case AttachmentLoadOp::clear:
            op = VK_ATTACHMENT_LOAD_OP_CLEAR;
            break;
        case AttachmentLoadOp::dont_care:
            op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            break;
        default:
            op = VK_ATTACHMENT_LOAD_OP_LOAD;
            break;
        }

        return op;
    }

    VkAttachmentStoreOp getAttachmentStoreOp(AttachmentStoreOp _op)
    {
        VkAttachmentStoreOp op = VK_ATTACHMENT_STORE_OP_STORE;

        switch (_op)
        {
        case AttachmentStoreOp::store:
            op = VK_ATTACHMENT_STORE_OP_STORE;
            break;
        case AttachmentStoreOp::dont_care:
            op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            break;
        case AttachmentStoreOp::none:
            op = VK_ATTACHMENT_STORE_OP_NONE;
            break;
        default:
            op = VK_ATTACHMENT_STORE_OP_STORE;
            break;
        }

        return op;
    }

    const char* getVkAccessDebugName(VkAccessFlags _flags)
    {
        if (_flags == 0)
        {
            return "NONE";
        }

        static char buffer_1[1024];
        buffer_1[0] = 0;

        if (_flags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT) {
            strcat(buffer_1, "INDIRECT_COMMAND_READ");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_INDEX_READ_BIT) {
            strcat(buffer_1, "INDEX_READ");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT) {
            strcat(buffer_1, "VERTEX_ATTRIBUTE_READ");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_UNIFORM_READ_BIT) {
            strcat(buffer_1, "UNIFORM_READ");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) {
            strcat(buffer_1, "INPUT_ATTACHMENT_READ");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_SHADER_READ_BIT) {
            strcat(buffer_1, "SHADER_READ");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_SHADER_WRITE_BIT) {
            strcat(buffer_1, "SHADER_WRITE");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_COLOR_ATTACHMENT_READ_BIT) {
            strcat(buffer_1, "COLOR_ATTACHMENT_READ");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) {
            strcat(buffer_1, "COLOR_ATTACHMENT_WRITE");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT) {
            strcat(buffer_1, "DEPTH_STENCIL_ATTACHMENT_READ");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT) {
            strcat(buffer_1, "DEPTH_STENCIL_ATTACHMENT_WRITE");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_TRANSFER_READ_BIT) {
            strcat(buffer_1, "TRANSFER_READ");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_TRANSFER_WRITE_BIT) {
            strcat(buffer_1, "TRANSFER_WRITE");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_HOST_READ_BIT) {
            strcat(buffer_1, "HOST_READ");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_HOST_WRITE_BIT) {
            strcat(buffer_1, "HOST_WRITE");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_MEMORY_READ_BIT) {
            strcat(buffer_1, "MEMORY_READ");
            strcat(buffer_1, "|");
        }
        if (_flags & VK_ACCESS_MEMORY_WRITE_BIT) {
            strcat(buffer_1, "MEMORY_WRITE");
            strcat(buffer_1, "|");
        }
        // set the final '|' to '\0'
        buffer_1[strlen(buffer_1) - 1] = 0;

        return buffer_1;

    }

    const char* getVkPipelineStageDebugName(VkPipelineStageFlags _flags)
    {
        if (_flags == 0)
        {
            return "NONE";
        }

        static char buffer_0[1024];
        buffer_0[0] = 0;
        if (_flags & VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
            strcat(buffer_0, "TOP_OF_PIPE");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT) {
            strcat(buffer_0, "DRAW_INDIRECT");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_VERTEX_INPUT_BIT) {
            strcat(buffer_0, "VERTEX_INPUT");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) {
            strcat(buffer_0, "VERTEX_SHADER");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT) {
            strcat(buffer_0, "TESSELLATION_CONTROL_SHADER");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT) {
            strcat(buffer_0, "TESSELLATION_EVALUATION_SHADER");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT) {
            strcat(buffer_0, "GEOMETRY_SHADER");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) {
            strcat(buffer_0, "FRAGMENT_SHADER");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT) {
            strcat(buffer_0, "EARLY_FRAGMENT_TESTS");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT) {
            strcat(buffer_0, "LATE_FRAGMENT_TESTS");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) {
            strcat(buffer_0, "COLOR_ATTACHMENT_OUTPUT");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
            strcat(buffer_0, "COMPUTE_SHADER");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_TRANSFER_BIT) {
            strcat(buffer_0, "TRANSFER");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT) {
            strcat(buffer_0, "BOTTOM_OF_PIPE");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_HOST_BIT) {
            strcat(buffer_0, "HOST");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT) {
            strcat(buffer_0, "ALL_GRAPHICS");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
            strcat(buffer_0, "ALL_COMMANDS");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT) {
            strcat(buffer_0, "TRANSFORM_FEEDBACK");
            strcat(buffer_0, "|");
        }
        if (_flags & VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT) {
            strcat(buffer_0, "CONDITIONAL_RENDERING");
            strcat(buffer_0, "|");
        }

        // set the final '|' to '\0'
        buffer_0[strlen(buffer_0) - 1] = 0;

        return buffer_0;
    }

    const char* getVkImageLayoutDebugName(VkImageLayout _layout)
    {
        if (_layout == 0)
        {
            return "UNDEFINED";
        }

        if (_layout == VK_IMAGE_LAYOUT_GENERAL) {
            return "GENERAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            return "COLOR_ATTACHMENT_OPTIMAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            return "DEPTH_STENCIL_ATTACHMENT_OPTIMAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
            return "DEPTH_STENCIL_READ_ONLY_OPTIMAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            return "SHADER_READ_ONLY_OPTIMAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            return "TRANSFER_SRC_OPTIMAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            return "TRANSFER_DST_OPTIMAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
            return "PREINITIALIZED";
        }
        if (_layout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL) {
            return "DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL) {
            return "DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
            return "DEPTH_ATTACHMENT_OPTIMAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) {
            return "DEPTH_READ_ONLY_OPTIMAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL) {
            return "STENCIL_ATTACHMENT_OPTIMAL";
        }
        if (_layout == VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL) {
            return "STENCIL_READ_ONLY_OPTIMAL";
        }

        return "UNDEFINED";
    }

    bool getVertexInputState(VkPipelineVertexInputStateCreateInfo& _out, const stl::vector<VertexBindingDesc>& _bindings, const stl::vector<VertexAttributeDesc>& _attributes)
    {
        if (_bindings.empty() && _attributes.empty())
        {
            return false;
        }

        stl::vector<VkVertexInputBindingDescription> bindings(_bindings.size());
        for (uint32_t ii = 0; ii < _bindings.size(); ++ ii)
        {
            const VertexBindingDesc& bind = _bindings[ii];
            bindings[ii] = { bind.binding, bind.stride, getInputRate(bind.inputRate) };
        }

        stl::vector<VkVertexInputAttributeDescription> attributes(_attributes.size());
        for (uint32_t ii = 0; ii < _attributes.size(); ++ii)
        {
            const VertexAttributeDesc& attr = _attributes[ii];
            attributes[ii] = { attr.location, attr.binding, getFormat(attr.format), attr.offset };
        }

        const kage::Memory* vtxBindingMem = kage::alloc(uint32_t(sizeof(VkVertexInputBindingDescription) * bindings.size()));
        memcpy(vtxBindingMem->data, bindings.data(), sizeof(VkVertexInputBindingDescription) * bindings.size());

        const kage::Memory* vtxAttributeMem = kage::alloc(uint32_t(sizeof(VkVertexInputAttributeDescription) * attributes.size()));
        memcpy(vtxAttributeMem->data, attributes.data(), sizeof(VkVertexInputAttributeDescription) * attributes.size());

        _out.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        _out.vertexBindingDescriptionCount = (uint32_t)bindings.size();
        _out.pVertexBindingDescriptions = (VkVertexInputBindingDescription*)vtxBindingMem->data;
        _out.vertexAttributeDescriptionCount = (uint32_t)attributes.size();
        _out.pVertexAttributeDescriptions = (VkVertexInputAttributeDescription*)vtxAttributeMem->data;

        return true;
    }

    void enumrateDeviceExtPorps(VkPhysicalDevice physicalDevice, stl::vector<VkExtensionProperties>& availableExtensions)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        availableExtensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
    }

    bool checkExtSupportness(const stl::vector<VkExtensionProperties>& props, const char* extName, bool print = true)
    {
        bool extSupported = false;
        for (const auto& extension : props) {
            if (strcmp(extension.extensionName, extName) != 0) {
                extSupported = true;
                break;
            }
        }

        if (print)
        {
            kage::message(kage::info, "%s : %s\n", extName, extSupported ? "true" : "false");
        }


        return extSupported;
    }

    ImgInitProps_vk getImageInitProp(const ImageCreateInfo& info, VkFormat _defaultColorFormat, VkFormat _defaultDepthFormat)
    {
        ImgInitProps_vk props{};
        props.numMips = info.numMips;
        props.numLayers = info.numLayers;
        props.width = info.width;
        props.height = info.height;
        props.depth = info.depth;
        props.format = getValidFormat(info.format, _defaultColorFormat, _defaultDepthFormat);
        props.usage = getImageUsageFlags(info.usage);
        props.type = getImageType(info.type);
        props.layout = getImageLayout(info.layout);
        props.viewType = getImageViewType(info.viewType);
        props.aspectMask = getImageAspectFlags(info.aspectFlags);

        return props;
    }

    VkQueryPool createQueryPool(VkDevice device, uint32_t queryCount, VkQueryType queryType)
    {
        VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
        createInfo.queryType = queryType;
        createInfo.queryCount = queryCount;

        if (queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS)
        {
            createInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT;
        }

        VkQueryPool queryPool = nullptr;
        VK_CHECK(vkCreateQueryPool(device, &createInfo, 0, &queryPool));
        return queryPool;
    }

    template<typename Ty>
    constexpr VkObjectType getType();

    template<> VkObjectType getType<VkBuffer             >() { return VK_OBJECT_TYPE_BUFFER; }
    template<> VkObjectType getType<VkCommandPool        >() { return VK_OBJECT_TYPE_COMMAND_POOL; }
    template<> VkObjectType getType<VkDescriptorPool     >() { return VK_OBJECT_TYPE_DESCRIPTOR_POOL; }
    template<> VkObjectType getType<VkDescriptorSet      >() { return VK_OBJECT_TYPE_DESCRIPTOR_SET; }
    template<> VkObjectType getType<VkDescriptorSetLayout>() { return VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT; }
    template<> VkObjectType getType<VkDeviceMemory       >() { return VK_OBJECT_TYPE_DEVICE_MEMORY; }
    template<> VkObjectType getType<VkFence              >() { return VK_OBJECT_TYPE_FENCE; }
    template<> VkObjectType getType<VkFramebuffer        >() { return VK_OBJECT_TYPE_FRAMEBUFFER; }
    template<> VkObjectType getType<VkImage              >() { return VK_OBJECT_TYPE_IMAGE; }
    template<> VkObjectType getType<VkImageView          >() { return VK_OBJECT_TYPE_IMAGE_VIEW; }
    template<> VkObjectType getType<VkPipeline           >() { return VK_OBJECT_TYPE_PIPELINE; }
    template<> VkObjectType getType<VkPipelineCache      >() { return VK_OBJECT_TYPE_PIPELINE_CACHE; }
    template<> VkObjectType getType<VkPipelineLayout     >() { return VK_OBJECT_TYPE_PIPELINE_LAYOUT; }
    template<> VkObjectType getType<VkQueryPool          >() { return VK_OBJECT_TYPE_QUERY_POOL; }
    template<> VkObjectType getType<VkRenderPass         >() { return VK_OBJECT_TYPE_RENDER_PASS; }
    template<> VkObjectType getType<VkSampler            >() { return VK_OBJECT_TYPE_SAMPLER; }
    template<> VkObjectType getType<VkSemaphore          >() { return VK_OBJECT_TYPE_SEMAPHORE; }
    template<> VkObjectType getType<VkShaderModule       >() { return VK_OBJECT_TYPE_SHADER_MODULE; }
    template<> VkObjectType getType<VkSurfaceKHR         >() { return VK_OBJECT_TYPE_SURFACE_KHR; }
    template<> VkObjectType getType<VkSwapchainKHR       >() { return VK_OBJECT_TYPE_SWAPCHAIN_KHR; }

    template<typename Ty>
    static void setDebugObjName(VkDevice _device, Ty _obj, const char* _format, ...)
    {
        if (BX_ENABLED(KAGE_DEBUG))
        {
            va_list args;
            va_start(args, _format);
            char str[2048];

            int32_t size = bx::min<int32_t>(sizeof(str) - 1, bx::vsnprintf(str, sizeof(str), _format, args));
            va_end(args);

            setLocalDebugName(_obj, str);

            VkDebugUtilsObjectNameInfoEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
            info.objectType = getType<Ty>();
            info.objectHandle = (uint64_t)(_obj.vk);
            info.pObjectName = str;

            VK_CHECK(vkSetDebugUtilsObjectNameEXT(_device, &info));
        }
    }

    template<typename Ty>
    static void refreshDebugNameObject(VkDevice _device, Ty _oldObj, Ty _newObj)
    {

        if (BX_ENABLED(KAGE_DEBUG))
        {
            const char* name = getLocalDebugName(_oldObj);
            if (name)
            {
                setDebugObjName(_device, _newObj, "%s", name);
                unsetLocalDebugName(_oldObj);
            }
        }
    }

    RHIContext_vk::RHIContext_vk(bx::AllocatorI* _allocator)
        : RHIContext(_allocator)
        , m_nwh(nullptr)
        , m_instance{ VK_NULL_HANDLE }
        , m_device{ VK_NULL_HANDLE }
        , m_physicalDevice{ VK_NULL_HANDLE }
        , m_surface{ VK_NULL_HANDLE }
        , m_queue{ VK_NULL_HANDLE }
        , m_cmdBuffer{ VK_NULL_HANDLE }
        , m_swapchainFormat{ VK_FORMAT_UNDEFINED }
        , m_depthFormat{ VK_FORMAT_UNDEFINED }
        , m_gfxFamilyIdx{ VK_QUEUE_FAMILY_IGNORED }
        , m_allocatorCb{nullptr}
        , m_debugCallback{ VK_NULL_HANDLE }
    {
        KG_ZoneScopedC(Color::indian_red);
    }

    RHIContext_vk::~RHIContext_vk()
    {
        KG_ZoneScopedC(Color::indian_red);

        shutdown();
    }

    void RHIContext_vk::init(const Resolution& _resolution, void* _wnd)
    {
        KG_ZoneScopedC(Color::indian_red);

        VK_CHECK(volkInitialize());

        this->createInstance();

        if (BX_ENABLED(KAGE_DEBUG))
        {
            m_debugCallback = registerDebugCallback(m_instance);
        }

        createPhysicalDevice();

        stl::vector<VkExtensionProperties> supportedExtensions;
        enumrateDeviceExtPorps(m_physicalDevice, supportedExtensions);

        m_supportMeshShading = checkExtSupportness(supportedExtensions, VK_EXT_MESH_SHADER_EXTENSION_NAME, false);

        vkGetPhysicalDeviceProperties(m_physicalDevice, &m_phyDeviceProps);
        assert(m_phyDeviceProps.limits.timestampPeriod);

        m_gfxFamilyIdx = getGraphicsFamilyIndex(m_physicalDevice);
        assert(m_gfxFamilyIdx != VK_QUEUE_FAMILY_IGNORED);

        m_device = kage::vk::createDevice(m_instance, m_physicalDevice, m_gfxFamilyIdx, m_supportMeshShading);
        assert(m_device);
        
        // only single device used in this application.
        volkLoadDevice(m_device);



        m_nwh = _wnd;

        m_swapchain.create(m_nwh, _resolution);

        m_swapchainFormat = m_swapchain.getSwapchainFormat();
        m_depthFormat = VK_FORMAT_D32_SFLOAT;

        // fill the image to barrier
        for (uint32_t ii = 0; ii < m_swapchain.m_swapchainImageCount; ++ii)
        {
            VkImage scimg = m_swapchain.m_swapchainImages[ii];
            setDebugObjName(m_device, scimg, "Img_Swapchain_%d", ii);

            m_barrierDispatcher.track(
                scimg
                , VK_IMAGE_ASPECT_COLOR_BIT
                , { 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
            );
        }

        vkGetDeviceQueue(m_device, m_gfxFamilyIdx, 0, &m_queue);
        assert(m_queue);

        initTracy(m_queue, m_gfxFamilyIdx);

        {
            m_numFramesInFlight = _resolution.maxFrameLatency == 0
                ? kMaxNumFrameLatency
                : _resolution.maxFrameLatency
                ;

            m_cmd.init(m_gfxFamilyIdx, m_queue, m_numFramesInFlight);

            m_cmd.alloc(&m_cmdBuffer);
        }

        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memProps);

        for (uint32_t ii = 0; ii < m_numFramesInFlight; ++ii)
        {
            m_scratchBuffer[ii].create(128, kMaxDrawCalls);
        }

        m_frameRecCmds.init();

        // get the function pointers
        m_vkGetDeviceProcAddr = vkGetDeviceProcAddr;
        initFFX();
    }

    void RHIContext_vk::bake()
    {
        KG_ZoneScopedC(Color::indian_red);

        RHIContext::bake();

        m_queryTimeStampCount = (uint32_t)(2 * m_passContainer.size());
        m_queryPoolTimeStamp = createQueryPool(m_device, m_queryTimeStampCount, VK_QUERY_TYPE_TIMESTAMP);
        assert(m_queryPoolTimeStamp);

        m_queryStatisticsCount = (uint32_t)(2 * m_passContainer.size());
        m_queryPoolStatistics = createQueryPool(m_device, m_queryStatisticsCount, VK_QUERY_TYPE_PIPELINE_STATISTICS);
        assert(m_queryPoolStatistics);
    }

    bool RHIContext_vk::run()
    {
        m_frameRecCmds.finish();

        bool result = render();

        m_frameRecCmds.start();

        return result;
    }

    bool RHIContext_vk::render()
    {
        KG_ZoneScopedC(Color::indian_red);

        if (m_passContainer.size() < 1)
        {
            message(DebugMsgType::error, "no pass needs to execute in context!");
            return false;
        }

        if (!m_swapchain.acquire(m_cmdBuffer))
        {
            return true;
        }

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        // time stamp
        uint32_t queryIdx = 0;

        // zone the command buffer to fix tracy tracking issue
        {
            KG_VkZoneC(m_tracyVkCtx, m_cmdBuffer, "main command buffer", Color::blue);

            vkCmdResetQueryPool(m_cmdBuffer, m_queryPoolStatistics, 0, m_queryStatisticsCount);

            vkCmdResetQueryPool(m_cmdBuffer, m_queryPoolTimeStamp, 0, m_queryTimeStampCount);
            vkCmdWriteTimestamp(m_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_queryPoolTimeStamp, 0);
            
            // render passes
            for (size_t ii = 0; ii < m_passContainer.size(); ++ii)
            {
                uint16_t passId = m_passContainer.getIdAt(ii);
                PassHandle p = PassHandle{ passId };
                const char* pn = getName(p);
                KG_VkZoneTransient(m_tracyVkCtx, var, m_cmdBuffer, pn);
                message(info, "==== start pass : %s", pn);

                vkCmdBeginQuery(m_cmdBuffer, m_queryPoolStatistics, (uint32_t)ii, 0);

                createBarriers(passId);

                executePass(passId);

                // will dispatch barriers internally
                flushWriteBarriers(passId);

                vkCmdEndQuery(m_cmdBuffer, m_queryPoolStatistics, (uint32_t)ii);

                // write time stamp
                vkCmdWriteTimestamp(m_cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPoolTimeStamp, (uint32_t)(ii + 1));
                message(info, "==== end pass : %s", pn);
            }

            // to swapchain
            drawToSwapchain(m_swapchain.m_swapchainImageIndex);
        }

        m_cmd.addWaitSemaphore(m_swapchain.m_waitSemaphore, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        m_cmd.addSignalSemaphore(m_swapchain.m_signalSemaphore);

        m_cmd.kick();
        m_cmd.alloc(&m_cmdBuffer);

        m_swapchain.present();
        m_cmd.finish();

        // wait
        {
            KG_ZoneScopedNC("wait", Color::blue);
            VK_CHECK(vkDeviceWaitIdle(m_device)); // TODO: a fence here?
        }

        // set the statistic data

        stl::vector<uint64_t> statistics(m_passContainer.size());
        VK_CHECK(vkGetQueryPoolResults(
            m_device
            , m_queryPoolStatistics
            , 0
            , (uint32_t)statistics.size()
            , sizeof(uint64_t) * statistics.size()
            , statistics.data()
            , sizeof(uint64_t)
            , VK_QUERY_RESULT_64_BIT
        ));
        m_passStatistics.clear();
        for (uint32_t ii = 0; ii < m_passContainer.size(); ++ii)
        {
            uint16_t passId = m_passContainer.getIdAt(ii);
            uint64_t clippedCount = statistics[ii];

            m_passStatistics.insert({ passId, clippedCount });
        }


        // set the time stamp data
        stl::vector<uint64_t> timeStamps(m_passContainer.size() + 1);
        VK_CHECK(vkGetQueryPoolResults(m_device, m_queryPoolTimeStamp, 0, (uint32_t)timeStamps.size(), sizeof(uint64_t) * timeStamps.size(), timeStamps.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT));
        m_passTime.clear();
        for (uint32_t ii = 0; ii < timeStamps.size() - 1; ++ii)
        {
            uint16_t passId = m_passContainer.getIdAt(ii);
            double timeStart = double(timeStamps[ii]) * m_phyDeviceProps.limits.timestampPeriod * 1e-6;
            double timeEnd = double(timeStamps[ii+1]) * m_phyDeviceProps.limits.timestampPeriod * 1e-6;

            m_passTime.insert({ passId, timeEnd - timeStart });
        }
        double gpuTimeStart = double(timeStamps[0]) * m_phyDeviceProps.limits.timestampPeriod * 1e-6;
        double gpuTimeEnd = double(timeStamps.back()) * m_phyDeviceProps.limits.timestampPeriod * 1e-6;
        m_gpuTime = gpuTimeEnd - gpuTimeStart;

        return true;
    }

    void RHIContext_vk::kick(bool _finishAll /*= false*/)
    {
        m_cmd.kick(_finishAll);
        m_cmd.alloc(&m_cmdBuffer);
        m_cmd.finish(_finishAll);
    }

    void RHIContext_vk::shutdown()
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_device)
        {
            return;
        }

        vkDeviceWaitIdle(m_device);

        if (m_queryPoolTimeStamp)
        {
            vkDestroyQueryPool(m_device, m_queryPoolTimeStamp, 0);
            m_queryPoolTimeStamp = VK_NULL_HANDLE;
        }

        if (m_queryPoolStatistics)
        {
            vkDestroyQueryPool(m_device, m_queryPoolStatistics, 0);
            m_queryPoolStatistics = VK_NULL_HANDLE;
        }

        for (VkDescriptorPool& pool : m_descPools)
        {
            vkDestroyDescriptorPool(m_device, pool, 0);
            pool = VK_NULL_HANDLE;
        }
        m_descPools.clear();

        destroyTracy();

        if (m_cmdBuffer)
        {
            m_cmdBuffer = VK_NULL_HANDLE;
        }

        m_swapchain.destroy();

        /// containers
        // buffer
        for (uint32_t ii = 0; ii < m_bufferContainer.size(); ++ii)
        {
            uint16_t bufId = m_bufferContainer.getIdAt(ii);
            Buffer_vk& buf = m_bufferContainer.getDataRef(bufId);
            if (buf.buffer)
            {
                vkDestroyBuffer(m_device, buf.buffer, nullptr);
                buf.buffer = VK_NULL_HANDLE;
            }

            if (buf.memory)
            {
                vkFreeMemory(m_device, buf.memory, nullptr);
                buf.memory = VK_NULL_HANDLE;
            }

            m_imgViewCache.invalidateWithParent(bufId);
        }
        m_bufferContainer.clear();

        // image
        for (uint32_t ii = 0; ii < m_imageContainer.size(); ++ii)
        {
            uint16_t imgId = m_imageContainer.getIdAt(ii);
            Image_vk& img = m_imageContainer.getDataRef(imgId);
            if (img.image)
            {
                vkDestroyImage(m_device, img.image, nullptr);
                img.image = VK_NULL_HANDLE;
            }

            if (img.memory)
            {
                vkFreeMemory(m_device, img.memory, nullptr);
                img.memory = VK_NULL_HANDLE;
            }

            if (img.defaultView)
            {
                vkDestroyImageView(m_device, img.defaultView, nullptr);
                img.defaultView = VK_NULL_HANDLE;
            }
            m_imgViewCache.invalidateWithParent(imgId);
        }
        m_imageContainer.clear();


        // samplers
        for (uint32_t ii = 0; ii < m_samplerContainer.size(); ++ii)
        {
            uint16_t samplerId = m_samplerContainer.getIdAt(ii);
            VkSampler& sampler = m_samplerContainer.getDataRef(samplerId);
            if (sampler)
            {
                vkDestroySampler(m_device, sampler, nullptr);
                sampler = VK_NULL_HANDLE;
            }
        }

        // render pass
        for (uint32_t ii = 0; ii < m_passContainer.size(); ++ii)
        {
            uint16_t passId = m_passContainer.getIdAt(ii);
            PassInfo_vk& pass = m_passContainer.getDataRef(passId);

            if (m_device && pass.pipeline)
            {
                vkDestroyPipeline(m_device, pass.pipeline, nullptr);
                pass.pipeline = VK_NULL_HANDLE;
            }
        }
        m_passContainer.clear();

        // shader
        for (uint32_t ii = 0; ii < m_shaderContainer.size(); ++ii)
        {
            uint16_t shaderId = m_shaderContainer.getIdAt(ii);
            Shader_vk& shader = m_shaderContainer.getDataRef(shaderId);
            if (shader.module)
            {
                vkDestroyShaderModule(m_device, shader.module, nullptr);
                shader.module = VK_NULL_HANDLE;
            }
        }
        m_shaderContainer.clear();

        // program
        for (uint32_t ii = 0; ii < m_programContainer.size(); ++ii)
        {
            uint16_t programId = m_programContainer.getIdAt(ii);
            Program_vk& program = m_programContainer.getDataRef(programId);
            if (program.layout)
            {
                vkDestroyPipelineLayout(m_device, program.layout, nullptr);
                program.layout = VK_NULL_HANDLE;
            }
        }

        if (m_device)
        {
            vkDestroyDevice(m_device, 0);
            m_device = VK_NULL_HANDLE;
        }
        
        if (m_instance)
        {
            vkDestroyInstance(m_instance, 0);
            m_instance = VK_NULL_HANDLE;
        }

        // descriptor set layout
        m_bufferCreateInfos.clear();
        m_imgCreateInfos.clear();

        m_programShaderIds.clear();
        m_progThreadCount.clear();

        m_aliasToBaseBuffers.clear();
        m_aliasToBaseImages.clear();
    }

    bool RHIContext_vk::checkSupports(VulkanSupportExtension _ext)
    {
        KG_ZoneScopedC(Color::indian_red);
        const char* extName = getExtName(_ext);
        stl::vector<VkExtensionProperties> supportedExtensions;
        enumrateDeviceExtPorps(m_physicalDevice, supportedExtensions);

        return checkExtSupportness(supportedExtensions, extName);
    }

    void RHIContext_vk::updateResolution(const Resolution& _resolution)
    {
        KG_ZoneScopedC(Color::indian_red);
        if (_resolution.width != m_resolution.width
            || _resolution.height != m_resolution.height
            || _resolution.reset != m_resolution.reset
            || m_swapchain.m_shouldRecreateSwapchain
            )
        {
            // untrack old swapchain images
            for (uint32_t ii = 0; ii < m_swapchain.m_swapchainImageCount; ++ii)
            {
                unsetLocalDebugName(m_swapchain.m_swapchainImages[ii]);
                m_barrierDispatcher.untrack(m_swapchain.m_swapchainImages[ii]);
            }

            m_swapchain.update(m_nwh, _resolution);

            // track new images
            for (uint32_t ii = 0; ii < m_swapchain.m_swapchainImageCount; ++ii)
            {
                VkImage scimg = m_swapchain.m_swapchainImages[ii];
                setDebugObjName(m_device, scimg, "Img_Swapchain_%d", ii);

                m_barrierDispatcher.track(
                    m_swapchain.m_swapchainImages[ii]
                    , VK_IMAGE_ASPECT_COLOR_BIT
                    , { 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
                );
            }

            // update attachments
            for (const kage::ImageHandle & hImg : m_colorAttchBase)
            {
                updateImage(hImg, _resolution.width, _resolution.height, nullptr);
            }

            for (const kage::ImageHandle & hImg : m_depthAttchBase)
            {
                updateImage(hImg, _resolution.width, _resolution.height, nullptr);
            }

            m_resolution = _resolution;
            message(info,
                "update m_resolution: width: %d, height: %d, reset: %016x"
                , m_resolution.width
                , m_resolution.height
                , m_resolution.reset
            );

            m_updated = true;
        }
    }

    void RHIContext_vk::updateBuffer(
        const BufferHandle _hBuf
        , const Memory* _mem
        , const uint32_t _offset
        , const uint32_t _size
    )
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_bufferContainer.exist(_hBuf.id))
        {
            message(info, "updateBuffer will not perform for buffer %d! It might be useless after render pass sorted", _hBuf.id);
            return;
        }

        const Buffer_vk& buf = m_bufferContainer.getIdToData(_hBuf.id);
        uint16_t baseBufId = m_aliasToBaseBuffers.getIdToData(_hBuf.id);
        const Buffer_vk& baseBuf = m_bufferContainer.getIdToData(baseBufId);

        const BufferCreateInfo& createInfo = m_bufferCreateInfos.getDataRef(baseBufId);
        if (!(createInfo.memFlags & MemoryPropFlagBits::host_visible))
        {
            message(info, "updateBuffer will not perform for buffer %d! Buffer must be host visible so we can map to host memory", _hBuf.id);
            return;
        }

        // re-create buffer if new size is larger than the old one
        if (buf.size != _mem->size && baseBuf.size < _mem->size)
        {
            m_barrierDispatcher.untrack(buf.buffer);

            Buffer_vk& bufToRelease = m_bufferContainer.getDataRef(_hBuf.id);
            release(bufToRelease.buffer);
            release(bufToRelease.memory);

            // recreate buffer
            BufferAliasInfo info{};
            info.size = _mem->size;
            info.bufId = _hBuf.id;

            Buffer_vk newBuf = kage::vk::createBuffer(
                info
                , getBufferUsageFlags(createInfo.usage)
                , getMemPropFlags(createInfo.memFlags)
                , getFormat(createInfo.format)
            );
            m_bufferContainer.update(_hBuf.id, newBuf);

            ResInteractDesc interact{ createInfo.barrierState };
            m_barrierDispatcher.track(
                newBuf.buffer
                , { getAccessFlags(interact.access), getPipelineStageFlags(interact.stage) }
                , newBuf.buffer
            );
        }

        const Buffer_vk& newBuf = m_bufferContainer.getIdToData(_hBuf.id);
        uploadBuffer(_hBuf.id, _mem->data, _mem->size);


        if (0 == (createInfo.memFlags & MemoryPropFlagBits::host_coherent))
        {
            flushBuffer(newBuf);
        }

    }

    void RHIContext_vk::updateImage(
        const ImageHandle _hImg
        , const uint16_t _width
        , const uint16_t _height
        , const Memory* _mem
    )
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_imageContainer.exist(_hImg.id))
        {
            message(info, "updateImage will not perform for image %d! It might be useless after render pass sorted", _hImg.id);
            return;
        }

        ImageHandle baseImg = { m_aliasToBaseImages.getIdToData(_hImg.id) };
        const stl::vector<ImageHandle>& aliasRef = m_imgToAliases.find(baseImg)->second;

        updateImageWithAlias(_hImg, _width, _height, _mem, aliasRef);
    }

    void RHIContext_vk::updateImageWithAlias(
        const ImageHandle _hImg
        , const uint16_t _width
        , const uint16_t _height
        , const Memory* _mem
        , const stl::vector<ImageHandle>& _alias
    )
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_imageContainer.exist(_hImg.id))
        {
            message(info, "updateImage will not perform for image %d! It might be useless after render pass sorted", _hImg.id);
            return;
        }

        const ImageHandle baseImg = { m_aliasToBaseImages.getIdToData(_hImg.id) };
        Image_vk baseImgVk = m_imageContainer.getIdToData(baseImg.id);

        // re-create image if new size is different from the old one
        if (baseImgVk.width != _width || baseImgVk.height != _height)
        {
            const BarrierState_vk baseBarrier = m_barrierDispatcher.getBaseBarrierState(baseImgVk.image);

            stl::vector<VkImage> toRelease;
            stl::vector<ImageAliasInfo> aliasInfos;
            for (const ImageHandle& img : _alias)
            {
                ImageAliasInfo ali{};
                ali.imgId = img.id;
                aliasInfos.push_back(ali);

                Image_vk& imgVk = m_imageContainer.getDataRef(img.id);
                toRelease.push_back(imgVk.image);
                
                m_barrierDispatcher.untrack(imgVk.image);
                
                message(info, "release vk image : %04x, vk: 0x%p", img.id, imgVk.image);
                
                release(imgVk.defaultView);
                release(imgVk.image);

                m_imgViewCache.invalidateWithParent(img.id);
            }

            release(baseImgVk.memory);

            ImageCreateInfo& ci = m_imgCreateInfos.getDataRef(_hImg.id);
            ci.width = _width;
            ci.height = _height;

            // recreate image
            ImgInitProps_vk initPorps = getImageInitProp(ci, m_swapchainFormat, m_depthFormat);

            stl::vector<Image_vk> imageVks;
            kage::vk::createImage(imageVks, aliasInfos, initPorps);
            assert(imageVks.size() == ci.resCount);

            ResInteractDesc interact{ ci.barrierState };
            for (uint16_t ii = 0; ii < _alias.size(); ++ii)
            {
                m_imageContainer.update({ _alias[ii].id }, imageVks[ii]);

                m_barrierDispatcher.track(
                    imageVks[ii].image
                    , getImageAspectFlags(initPorps.aspectMask)
                    , baseBarrier
                    , baseImgVk.image
                );

                refreshDebugNameObject(m_device, toRelease[ii], imageVks[ii].image);

                message(info, "update vk image : %04x, vk: 0x%p", _alias[ii].id, imageVks[ii].image);
            }
        }

        if (_mem != nullptr)
        {
            uploadImage(_hImg.id, _mem->data, _mem->size);
        }
    }

    void RHIContext_vk::createShader(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::indian_red);

        ShaderCreateInfo info;
        bx::read(&_reader, info, nullptr);

        char path[kMaxPathLen];
        bx::read(&_reader, path, info.pathLen, nullptr);
        path[info.pathLen] = '\0'; // null-terminated string

        Shader_vk shader{};
        bool lsr = loadShader(shader, m_device, path);
        assert(lsr);

        m_shaderContainer.addOrUpdate(info.shaderId, shader);
    }

    void RHIContext_vk::createProgram(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::indian_red);

        ProgramCreateInfo info;
        bx::read(&_reader, info, nullptr);

        VkDescriptorSetLayout setArrLayout = VK_NULL_HANDLE;
        if (kInvalidHandle != info.bindlessId)
        {
            setArrLayout = m_bindlessContainer.getIdToData(info.bindlessId).layout;
        }

        stl::vector<uint16_t> shaderIds(info.shaderNum);
        bx::read(&_reader, shaderIds.data(), info.shaderNum * sizeof(uint16_t), nullptr);

        stl::vector<Shader_vk> shaders;
        for (const uint16_t sid : shaderIds)
        {
            shaders.push_back(m_shaderContainer.getIdToData(sid));
        }

        VkPipelineBindPoint bindPoint = getBindPoint(shaders);
        Program_vk prog = kage::vk::createProgram(m_device, bindPoint, shaders, info.sizePushConstants, setArrLayout);

        m_programContainer.addOrUpdate(info.progId, prog);
        m_programShaderIds.emplace_back(shaderIds);

        assert(m_programContainer.size() == m_programShaderIds.size());
    }

    void RHIContext_vk::createPass(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::indian_red);

        PassMetaData passMeta;
        bx::read(&_reader, passMeta, nullptr);
        
        size_t vtxBindingSz = passMeta.vertexBindingNum * sizeof(VertexBindingDesc);
        size_t vtxAttributeSz = passMeta.vertexAttributeNum * sizeof(VertexAttributeDesc);

        stl::vector<VertexBindingDesc> passVertexBinding(passMeta.vertexBindingNum);
        bx::read(&_reader, passVertexBinding.data(), (int32_t)vtxBindingSz, nullptr);
        stl::vector<VertexAttributeDesc> passVertexAttribute(passMeta.vertexAttributeNum);
        bx::read(&_reader, passVertexAttribute.data(), (int32_t)vtxAttributeSz, nullptr);

        // pipeline spec data
        stl::vector<int> pipelineSpecData(passMeta.pipelineSpecNum);
        bx::read(&_reader, pipelineSpecData.data(), passMeta.pipelineSpecNum * sizeof(int), nullptr);

        // r/w resources
        stl::vector<uint16_t> writeImageIds(passMeta.writeImageNum);
        bx::read(&_reader, writeImageIds.data(), passMeta.writeImageNum * sizeof(uint16_t), nullptr);

        stl::vector<uint16_t> readImageIds(passMeta.readImageNum);
        bx::read(&_reader, readImageIds.data(), passMeta.readImageNum * sizeof(uint16_t), nullptr);

        stl::vector<uint16_t> readBufferIds(passMeta.readBufferNum);
        bx::read(&_reader, readBufferIds.data(), passMeta.readBufferNum * sizeof(uint16_t), nullptr);
        
        stl::vector<uint16_t> writeBufferIds(passMeta.writeBufferNum);
        bx::read(&_reader, writeBufferIds.data(), passMeta.writeBufferNum * sizeof(uint16_t), nullptr);

        const size_t interactSz =
            passMeta.writeImageNum +
            passMeta.readImageNum +
            passMeta.readBufferNum +
            passMeta.writeBufferNum;

        stl::vector<ResInteractDesc> interacts(interactSz);
        bx::read(&_reader, interacts.data(), (uint32_t)(interactSz * sizeof(ResInteractDesc)), nullptr);

        // write op aliases
        stl::vector<CombinedResID> writeOpAliasInIds(passMeta.writeBufAliasNum + passMeta.writeImgAliasNum);
        bx::read(&_reader, writeOpAliasInIds.data(), (passMeta.writeBufAliasNum + passMeta.writeImgAliasNum) * sizeof(CombinedResID), nullptr);

        stl::vector<CombinedResID> writeOpAliasOutIds(passMeta.writeBufAliasNum + passMeta.writeImgAliasNum);
        bx::read(&_reader, writeOpAliasOutIds.data(), (passMeta.writeBufAliasNum + passMeta.writeImgAliasNum) * sizeof(CombinedResID), nullptr);


        // fill pass info
        PassInfo_vk passInfo{};
        passInfo.passId = passMeta.passId;

        passInfo.vertexBufferId = passMeta.vertexBufferId;
        passInfo.vertexCount = passMeta.vertexCount;
        passInfo.indexBufferId = passMeta.indexBufferId;
        passInfo.indexCount = passMeta.indexCount;
        passInfo.writeDepthId = passMeta.writeDepthId;

        // desc part
        passInfo.programId = passMeta.programId;
        passInfo.queue = passMeta.queue;
        passInfo.vertexBindingNum = passMeta.vertexBindingNum;
        passInfo.vertexAttributeNum = passMeta.vertexAttributeNum;
        passInfo.vertexBindings = passMeta.vertexBindings;
        passInfo.vertexAttributes = passMeta.vertexAttributes;
        passInfo.pipelineSpecNum = passMeta.pipelineSpecNum;
        passInfo.pipelineSpecData = passMeta.pipelineSpecData;
        passInfo.pipelineConfig = passMeta.pipelineConfig;

        // r/w res part
        auto getBarrierState = [&](const ResInteractDesc& _desc) -> BarrierState_vk {
                BarrierState_vk state{};
                state.accessMask = getAccessFlags(_desc.access);
                state.imgLayout = getImageLayout(_desc.layout);
                state.stageMask = getPipelineStageFlags(_desc.stage);

                return state;
            };

        size_t offset = 0; // the depth is the first one
        auto preparePassBarriers = [&interacts, &offset, getBarrierState](
              const stl::vector<uint16_t>& _ids
            , ContinuousMap< uint16_t, BarrierState_vk>& _container
            , const ResourceType _type
            ) {
                for (uint32_t ii = 0; ii < _ids.size(); ++ii)
                {
                    _container.addOrUpdate(_ids[ii], getBarrierState(interacts[offset + ii]));
                } 

                offset += _ids.size();
            };

        const size_t depthIdx = getElemIndex(writeImageIds, passInfo.writeDepthId);
        if (depthIdx != kInvalidIndex)
        {
            passInfo.writeDepth = std::make_pair(passInfo.writeDepthId, getBarrierState(interacts[depthIdx]));
            writeImageIds.erase(writeImageIds.begin() + depthIdx);
            interacts.erase(interacts.begin() + depthIdx);
        }

        preparePassBarriers(writeImageIds, passInfo.writeColors, ResourceType::image);
        preparePassBarriers(readImageIds, passInfo.readImages, ResourceType::image);
        preparePassBarriers(readBufferIds, passInfo.readBuffers, ResourceType::buffer);
        preparePassBarriers(writeBufferIds, passInfo.writeBuffers, ResourceType::buffer);
        assert(offset == interacts.size());

        // write op alias part
        {
            size_t writeOpAliasCount = passMeta.writeBufAliasNum + passMeta.writeImgAliasNum;
            assert(writeOpAliasInIds.size() == writeOpAliasCount);
            assert(writeOpAliasOutIds.size() == writeOpAliasCount);
            for (uint32_t ii = 0; ii < writeOpAliasCount; ++ii )
            {
                passInfo.writeOpInToOut.addOrUpdate(writeOpAliasInIds[ii], writeOpAliasOutIds[ii]);
            }
        }

        VkPipeline pipeline{};
        if (passMeta.queue == PassExeQueue::graphics)
        {
            // create pipeline
            const uint16_t progIdx = (uint16_t)m_programContainer.getIdIndex(passInfo.programId);
            const Program_vk& program = m_programContainer.getIdToData(passInfo.programId);
            const stl::vector<uint16_t>& shaderIds = m_programShaderIds[progIdx];

            uint32_t depthNum = (passMeta.writeDepthId == kInvalidHandle) ? 0 : 1;

            stl::vector<VkFormat> colorFormats{};
            for (size_t ii = 0; ii < passInfo.writeColors.size(); ++ii)
            {
                uint16_t id = passInfo.writeColors.getIdAt(ii);

                const Image_vk& img = m_imageContainer.getIdToData(id);
                const BarrierState_vk& ba = passInfo.writeColors.getDataAt(ii);
                if ((ba.accessMask & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) == 0)
                    continue;

                colorFormats.emplace_back(img.format);
            }

            VkPipelineRenderingCreateInfo renderInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
            renderInfo.colorAttachmentCount = (uint32_t)colorFormats.size();
            renderInfo.pColorAttachmentFormats = colorFormats.data();
            renderInfo.depthAttachmentFormat = m_depthFormat;

            stl::vector< Shader_vk> shaders;
            for (const uint16_t sid : shaderIds)
            {
                shaders.push_back(m_shaderContainer.getIdToData(sid));
            }

            VkPipelineVertexInputStateCreateInfo vtxInputCreateInfo{};
            bool hasVIS = getVertexInputState(vtxInputCreateInfo, passVertexBinding, passVertexAttribute);

            PipelineConfigs_vk configs{
                passMeta.pipelineConfig.enableDepthTest
                , passMeta.pipelineConfig.enableDepthWrite
                , getCompareOp(passMeta.pipelineConfig.depthCompOp)
                , getCullMode(passMeta.pipelineConfig.cullMode)
                , getPolygonMode(passMeta.pipelineConfig.polygonMode)
            };

            VkPipelineCache cache{};
            cache.vk = 0;
            pipeline = kage::vk::createGraphicsPipeline(m_device, cache, program.layout, renderInfo, shaders, hasVIS ? &vtxInputCreateInfo : nullptr, pipelineSpecData, configs);
            assert(pipeline);
        }
        else if (passMeta.queue == PassExeQueue::compute)
        {
            // create pipeline
            const uint16_t progIdx = (uint16_t)m_programContainer.getIdIndex(passMeta.programId);
            const Program_vk& program = m_programContainer.getIdToData(passMeta.programId);
            const stl::vector<uint16_t>& shaderIds = m_programShaderIds[progIdx];

            stl::vector< Shader_vk> shaders;
            for (const uint16_t sid : shaderIds)
            {
                shaders.push_back(m_shaderContainer.getIdToData(sid));
            }

            assert(shaderIds.size() == 1);
            Shader_vk shader = m_shaderContainer.getIdToData(shaderIds[0]);

            VkPipelineCache cache{};
            cache.vk = 0;
            pipeline = kage::vk::createComputePipeline(m_device, cache, program.layout, shader, pipelineSpecData);
            assert(pipeline);
        }

        passInfo.pipeline = pipeline;
        m_passContainer.addOrUpdate(passInfo.passId, passInfo);
    }

    void RHIContext_vk::createImage(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::indian_red);

        ImageCreateInfo info;
        bx::read(&_reader, info, nullptr);

        ImageAliasInfo* resArr = new ImageAliasInfo[info.resCount];
        bx::read(&_reader, resArr, sizeof(ImageAliasInfo) * info.resCount, nullptr);

        // so the res handle should map to the real buffer array
        stl::vector<ImageAliasInfo> infoList(resArr, resArr + info.resCount);

        ImgInitProps_vk initPorps = getImageInitProp(info, m_swapchainFormat, m_depthFormat);

        stl::vector<Image_vk> images;
        kage::vk::createImage(images, infoList, initPorps);
        assert(images.size() == info.resCount);

        m_imgCreateInfos.addOrUpdate(info.imgId, info);

        for (int ii = 0; ii < info.resCount; ++ii)
        {
            m_imageContainer.addOrUpdate(resArr[ii].imgId, images[ii]);
            m_aliasToBaseImages.addOrUpdate(resArr[ii].imgId, info.imgId);
        }

        const Image_vk& baseImage = getImage(info.imgId, true);

        for (int ii = 0; ii < info.resCount; ++ii)
        {
            ResInteractDesc interact{ info.barrierState };

            m_barrierDispatcher.track(
                images[ii].image
                , getImageAspectFlags(images[ii].aspectMask)
                , { getAccessFlags(interact.access), getImageLayout(interact.layout) ,getPipelineStageFlags(interact.stage) }
                , baseImage.image
            );
        }

        {
            if (info.usage & ImageUsageFlagBits::color_attachment)
            {
                m_colorAttchBase.push_back({ info.imgId });
            }

            if (info.usage & ImageUsageFlagBits::depth_stencil_attachment)
            {
                m_depthAttchBase.push_back({ info.imgId });
            }

            if (info.usage & ImageUsageFlagBits::storage)
            {
                m_storageImageBase.push_back({ info.imgId });
            }

            stl::vector<ImageHandle> alias;
            for (uint16_t ii = 0; ii < info.resCount; ++ii)
            {
                alias.push_back({ resArr[ii].imgId });
            }

            m_imgToAliases.insert(
                stl::make_pair<ImageHandle, stl::vector<ImageHandle>>({ info.imgId }, alias)
            );
        }

        if (info.pData != nullptr)
        {
            uploadImage(info.imgId, info.pData, info.size);
        }

        KAGE_DELETE_ARRAY(resArr);
    }

    void RHIContext_vk::createBuffer(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::indian_red);

        BufferCreateInfo info;
        bx::read(&_reader, info, nullptr);

        BufferAliasInfo* resArr = new BufferAliasInfo[info.resCount];
        bx::read(&_reader, resArr, sizeof(BufferAliasInfo) * info.resCount, nullptr);

        // so the res handle should map to the real buffer array
        stl::vector<BufferAliasInfo> infoList(resArr, resArr + info.resCount);

        stl::vector<Buffer_vk> buffers;
        kage::vk::createBuffer(
            buffers
            , infoList
            , getBufferUsageFlags(info.usage)
            , getMemPropFlags(info.memFlags)
            , getFormat(info.format)
        );

        assert(buffers.size() == info.resCount);

        for (int ii = 0; ii < info.resCount; ++ii)
        {
            buffers[ii].fillVal = info.fillVal;
            m_bufferContainer.addOrUpdate(resArr[ii].bufId, buffers[ii]);
            m_aliasToBaseBuffers.addOrUpdate(resArr[ii].bufId, info.bufId);
        }

        m_bufferCreateInfos.addOrUpdate(info.bufId, info);

        const Buffer_vk& baseBuffer = getBuffer(info.bufId);
        for (int ii = 0; ii < info.resCount; ++ii)
        {
            ResInteractDesc interact{ info.barrierState };
            m_barrierDispatcher.track(
                buffers[ii].buffer
                , { getAccessFlags(interact.access), getPipelineStageFlags(interact.stage) }
                , baseBuffer.buffer
            );
        }
        
        // initialize buffer
        if (info.pData != nullptr)
        {
            uploadBuffer(info.bufId, info.pData, info.size);
        }
        else
        {
            fillBuffer(info.bufId, info.fillVal, info.size);
        }

        KAGE_DELETE_ARRAY(resArr);
    }

    void RHIContext_vk::createSampler(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::indian_red);

        SamplerMetaData meta;
        bx::read(&_reader, meta, nullptr);

        VkSampler sampler = getCachedSampler(
            meta.filter
            , meta.mipmapMode
            , meta.addressMode
            , meta.reductionMode
        );

        m_samplerContainer.addOrUpdate(meta.samplerId, sampler);
    }

    void RHIContext_vk::createBindless(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::indian_red);

        BindlessMetaData meta;
        bx::read(&_reader, meta, nullptr);
        assert(meta.resType == ResourceType::image);
        assert(meta.reductionMode != SamplerReductionMode::max_enum);

        uint32_t shaderCount;
        bx::read(&_reader, shaderCount, nullptr);

        stl::vector<uint16_t> shaderIds(shaderCount);
        bx::read(&_reader, shaderIds.data(), shaderCount * sizeof(uint16_t), nullptr);

        VkShaderStageFlags stages = 0;
        for (uint16_t sid : shaderIds)
        {
            const Shader_vk& shader = m_shaderContainer.getIdToData(sid);
            stages |= shader.stage;
        }

        uint32_t imgCnt = meta.resIdMem->size / sizeof(ImageHandle);
        stl::vector<ImageHandle> imageIds(imgCnt);
        memcpy(imageIds.data(), meta.resIdMem->data, meta.resIdMem->size);

        VkDescriptorPool pool = createDescriptorPool(m_device);
        assert(pool);
        VkDescriptorSetLayout layout = createDescArrayLayout(m_device, stages);
        assert(layout);

        uint32_t descCount = (uint32_t)imageIds.size() + 1;
        VkDescriptorSet set = createDescriptorSets(m_device, layout, pool, descCount);
        assert(set);

        Bindless_vk bindless{};
        bindless.pool = pool;
        bindless.layout = layout;
        bindless.set = set;
        bindless.setIdx = meta.setIdx;
        // TODO: where do the set Idx and set count come from?
        bindless.setCount = 1; 
        

        m_bindlessContainer.addOrUpdate(meta.bindlessId, bindless);
        m_descSetToPool.insert({ set, pool });

        const VkSampler& sampler = getCachedSampler(
            SamplerFilter::linear
            , SamplerMipmapMode::linear
            , SamplerAddressMode::repeat
            , meta.reductionMode
        );
        assert(sampler);

        for (size_t ii = 0; ii < imageIds.size(); ++ ii)
        {
            const ImageHandle hImg = imageIds[ii];

            if (!isValid(hImg))
            {
                message(warning, "imageIds[ii] is invalid! skipping");
                continue;
            }
            const Image_vk& img = m_imageContainer.getIdToData(hImg.id);

            
            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfo.imageView = img.defaultView;
            imgInfo.sampler = sampler;
            
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = set;
            write.dstBinding = meta.binding;
            write.dstArrayElement = uint32_t(ii + 1); // 0 is reserved for invalid image
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &imgInfo;
            vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);


            m_barrierDispatcher.barrier(
                img.image
                , img.aspectMask
                , { VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT }
            );
        }

        // dispatch barriers for all images as shader read only
        dispatchBarriers();
    }

    void RHIContext_vk::setBrief(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::indian_red);

        RHIBrief brief;
        bx::read(&_reader, brief, nullptr);

        m_brief = brief;
    }

    void RHIContext_vk::setName(Handle _h, const char* _name, uint32_t _len)
    {
        switch (_h.type)
        {
        case Handle::Shader:
            {
                if (m_shaderContainer.exist(_h.id))
                {
                    const Shader_vk& shader = m_shaderContainer.getIdToData(_h.id);
                    setDebugObjName(m_device, shader.module, "%.*s", _len, _name);
                }
            }
            break;
        case Handle::Program:
            {
                if (m_programContainer.exist(_h.id))
                {
                    const Program_vk& prog = m_programContainer.getIdToData(_h.id);
                    setDebugObjName(m_device, prog.layout, "%.*s", _len, _name);
                }
            }
            break;
        case Handle::Pass:
            {
                // do nothing here
                ;
            }
            break;
        case Handle::Buffer:
            {
                if (m_bufferContainer.exist(_h.id))
                {
                    const Buffer_vk& buf = m_bufferContainer.getIdToData(_h.id);
                    setDebugObjName(m_device, buf.buffer, "%.*s", _len, _name);
                }
            }
            break;
        case Handle::Image:
            {
                if (m_imageContainer.exist(_h.id))
                {
                    const Image_vk& img = m_imageContainer.getIdToData(_h.id);
                    setDebugObjName(m_device, img.image, "%.*s", _len, _name);
                }
            }
            break;
        default:
            BX_ASSERT(false, "invalid handle type? %d", _h.type);
            break;
        }
    }

    void RHIContext_vk::initFFX()
    {
        // Create the FFX context
        constexpr uint32_t c_maxContexts = 2;
        size_t scratchMemSz = ffxGetScratchMemorySizeVK(s_renderVK->m_physicalDevice, c_maxContexts);
        const kage::Memory* mem = kage::alloc((uint32_t)scratchMemSz);
        memset(mem->data, 0, mem->size);

        //void* mem = bx::alloc(entry::getAllocator(), scratchMemSz);

        VkDeviceContext vkdevCtx;
        vkdevCtx.vkDevice = s_renderVK->m_device;
        vkdevCtx.vkPhysicalDevice = s_renderVK->m_physicalDevice;
        vkdevCtx.vkDeviceProcAddr = s_renderVK->m_vkGetDeviceProcAddr;


        FfxDevice ffxDevice = ffxGetDeviceVK(&vkdevCtx);
        FfxErrorCode err = ffxGetInterfaceVK(&m_ffxInterface, ffxDevice, mem->data, mem->size, c_maxContexts);

        assert(err == FFX_OK);
    }

    void RHIContext_vk::setRecord(
        PassHandle _hPass
        , const CommandQueue& _cq
        , const uint32_t _offset
        , const uint32_t _size
    )
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "setRecord will not perform for pass %d[%s]! It might be useless after render pass sorted"
                , _hPass.id
                , getName(_hPass)
            );
            return;
        }

        m_frameRecCmds.record(_hPass, _cq, _offset, _size);
        PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        passInfo.recorded = true;
    }

    void RHIContext_vk::setConstants(PassHandle _hPass, const Memory* _mem)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "setConstants will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }

        const PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        const Program_vk& prog = m_programContainer.getIdToData(passInfo.programId);

        vkCmdPushConstants(
            m_cmdBuffer
            , prog.layout
            , prog.pushConstantStages
            , 0
            , _mem->size
            , _mem->data
        );
    }

    void RHIContext_vk::pushDescriptorSet(PassHandle _hPass, const Memory* _mem)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "setDescriptorSet will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }

        uint32_t count = _mem->size / sizeof(Binding);
        Binding* bds = (Binding*)_mem->data;
        m_descBindingSetsPerPass.assign(bds, bds + count);
    }

    void RHIContext_vk::setColorAttachments(PassHandle _hPass, const Memory* _mem)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "setDescriptorSet will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }

        uint32_t count = _mem->size / sizeof(Attachment);
        const Attachment* atts = (const Attachment*)_mem->data;

        m_colorAttachPerPass.assign(atts, atts + count);
    }

    void RHIContext_vk::setDepthAttachment(PassHandle _hPass, Attachment _depth)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "setDescriptorSet will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }

        m_depthAttachPerPass = _depth;
    }

    void RHIContext_vk::setBindless(PassHandle _hPass, BindlessHandle _hBindless)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "setBindless will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }

        // get program layout
        const PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        const Program_vk& prog = m_programContainer.getIdToData(passInfo.programId);
        prog.layout;
        
        // get bind point
        const Bindless_vk& bindless = m_bindlessContainer.getIdToData(_hBindless.id);

        vkCmdBindDescriptorSets(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prog.layout, bindless.setIdx, bindless.setCount, &bindless.set, 0, nullptr);
    }

    void RHIContext_vk::setViewport(PassHandle _hPass, int32_t _x, int32_t _y, uint32_t _w, uint32_t _h)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "setDescriptorSet will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }

        // flip the view port
        VkViewport viewport = { 
            float(_x)
            , float(_y+_h)
            , float(_w)
            , -float(_h)
            , 0.f
            , 1.f 
        };
        vkCmdSetViewport(m_cmdBuffer, 0, 1, &viewport);
    }

    void RHIContext_vk::setScissor(PassHandle _hPass, int32_t _x, int32_t _y, uint32_t _w, uint32_t _h)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "setDescriptorSet will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }

        VkRect2D scissor = { { _x, _y }, { _w, _h } };
        vkCmdSetScissor(m_cmdBuffer, 0, 1, &scissor);
    }

    void RHIContext_vk::setVertexBuffer(PassHandle _hPass, BufferHandle _hBuf)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "setVertexBuffer will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }
        const Buffer_vk& buf = getBuffer(_hBuf.id);

        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(m_cmdBuffer, 0, 1, &buf.buffer, offsets);
    }

    void RHIContext_vk::setIndexBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, IndexType _type)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "setIndexBuffer will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }

        const Buffer_vk& buf = getBuffer(_hBuf.id);
        VkIndexType t = getIndexType(_type);

        vkCmdBindIndexBuffer(m_cmdBuffer, buf.buffer, _offset, t);
    }

    void RHIContext_vk::fillBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _value)
    {
        KG_ZoneScopedC(Color::indian_red);
        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "fillBuffer will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }

        const Buffer_vk& buf = getBuffer(_hBuf.id);
        
        m_barrierDispatcher.barrier(buf.buffer,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        dispatchBarriers();

        vkCmdFillBuffer(m_cmdBuffer, buf.buffer, 0, VK_WHOLE_SIZE, _value);

        // write flush
        m_barrierDispatcher.barrier(buf.buffer,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        dispatchBarriers();
    }

    void RHIContext_vk::dispatch(PassHandle _hPass, uint32_t _x, uint32_t _y, uint32_t _z)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "dispatch will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }

        createBarriersRec(_hPass);
        lazySetDescriptorSet(_hPass);

        // get the dispatch size
        PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        const uint16_t progIdx = (uint16_t)m_programContainer.getIdIndex(passInfo.programId);
        const stl::vector<uint16_t>& shaderIds = m_programShaderIds[progIdx];
        assert(shaderIds.size() == 1);
        const Shader_vk& shader = m_shaderContainer.getIdToData(shaderIds[0]);

        // dispatch
        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, passInfo.pipeline);
        vkCmdDispatch(
            m_cmdBuffer
            , calcGroupCount(_x, shader.localSizeX)
            , calcGroupCount(_y, shader.localSizeY)
            , calcGroupCount(_z, shader.localSizeZ)
        );

        flushWriteBarriersRec(_hPass);
    }

    void RHIContext_vk::dispatchIndirect(PassHandle _hPass, BufferHandle _hIndirectBuf, uint32_t _offset)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "dispatch will not perform for pass %d! It might be useless after render pass sorted"
                , _hPass.id
            );
            return;
        }

        createBarriersRec(_hPass);
        lazySetDescriptorSet(_hPass);

        const PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        const Buffer_vk& ib = getBuffer(_hIndirectBuf.id);

        // dispatch
        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, passInfo.pipeline);
        vkCmdDispatchIndirect(
            m_cmdBuffer
            , ib.buffer
            , _offset
        );

        flushWriteBarriersRec(_hPass);
    }

    void RHIContext_vk::draw(PassHandle _hPass, uint32_t _vtxCount, uint32_t _instCount, uint32_t _firstIdx, uint32_t _firstInst)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "drawIndexed will not perform for pass %d! It might be cut after render pass sorted"
                , _hPass.id
            );
            return;
        }

        createBarriersRec(_hPass);
        lazySetDescriptorSet(_hPass);

        beginRendering(m_cmdBuffer, _hPass.id);

        PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        const uint16_t progIdx = (uint16_t)m_programContainer.getIdIndex(passInfo.programId);

        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passInfo.pipeline);

        vkCmdDraw(
            m_cmdBuffer
            , _vtxCount
            , _instCount
            , _firstIdx
            , _firstInst
        );

        endRendering(m_cmdBuffer);

        flushWriteBarriersRec(_hPass);
    }

    void RHIContext_vk::drawIndexed(
        PassHandle _hPass
        , uint32_t _idxCount
        , uint32_t _instCount
        , uint32_t _firstIdx
        , uint32_t _vtxOffset
        , uint32_t _firstInst
    )
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "drawIndexed will not perform for pass %d! It might be cut after render pass sorted"
                , _hPass.id
            );
            return;
        }

        createBarriersRec(_hPass);
        lazySetDescriptorSet(_hPass);

        beginRendering(m_cmdBuffer, _hPass.id);

        PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        const uint16_t progIdx = (uint16_t)m_programContainer.getIdIndex(passInfo.programId);

        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passInfo.pipeline);
        vkCmdDrawIndexed(
            m_cmdBuffer
            , _idxCount
            , _instCount
            , _firstIdx
            , _vtxOffset
            , _firstInst
        );

        endRendering(m_cmdBuffer);

        flushWriteBarriersRec(_hPass);
    }

    void RHIContext_vk::drawIndirect(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _drawCount)
    {
        BX_ASSERT(false, "not implemented");
    }

    void RHIContext_vk::drawIndexedIndirect(PassHandle _hPass, BufferHandle _hIndirectBuf, uint32_t _offset, uint32_t _drawCount, uint32_t _stride)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "drawIndexed will not perform for pass %d! It might be cut after render pass sorted"
                , _hPass.id
            );
            return;
        }

        createBarriersRec(_hPass);
        lazySetDescriptorSet(_hPass);

        beginRendering(m_cmdBuffer, _hPass.id);

        PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        const uint16_t progIdx = (uint16_t)m_programContainer.getIdIndex(passInfo.programId);

        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passInfo.pipeline);

        const Buffer_vk& ib = getBuffer(_hIndirectBuf.id);

        vkCmdDrawIndexedIndirect(
            m_cmdBuffer
            , ib.buffer
            , _offset
            , _drawCount
            , _stride
        );

        endRendering(m_cmdBuffer);

        flushWriteBarriersRec(_hPass);
    }

    void RHIContext_vk::drawIndexedIndirectCount(PassHandle _hPass, BufferHandle _hIndirectBuf, uint32_t _indirectOffset, BufferHandle _hIndirectCountBuf, uint32_t _countOffset, uint32_t _drawCount, uint32_t _stride)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "drawIndexed will not perform for pass %d! It might be cut after render pass sorted"
                , _hPass.id
            );
            return;
        }

        createBarriersRec(_hPass);
        lazySetDescriptorSet(_hPass);

        beginRendering(m_cmdBuffer, _hPass.id);

        PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        const uint16_t progIdx = (uint16_t)m_programContainer.getIdIndex(passInfo.programId);

        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passInfo.pipeline);

        const Buffer_vk& ib = getBuffer(_hIndirectBuf.id);
        const Buffer_vk& cb = getBuffer(_hIndirectCountBuf.id);

        vkCmdDrawIndexedIndirectCount(
            m_cmdBuffer
            , ib.buffer
            , _indirectOffset
            , cb.buffer
            , _countOffset
            , _drawCount
            , _stride
        );

        endRendering(m_cmdBuffer);

        flushWriteBarriersRec(_hPass);
    }

    void RHIContext_vk::drawMeshTaskIndirect(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _drawCount, uint32_t _stride)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(
                warning
                , "drawMeshTaskIndirect will not perform for pass %d! It might be cut after render pass sorted"
                , _hPass.id
            );
            return;
        }

        createBarriersRec(_hPass);
        lazySetDescriptorSet(_hPass);

        beginRendering(m_cmdBuffer, _hPass.id);

        PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        const uint16_t progIdx = (uint16_t)m_programContainer.getIdIndex(passInfo.programId);
        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passInfo.pipeline);

        const Buffer_vk& buf = getBuffer(_hBuf.id);
        vkCmdDrawMeshTasksIndirectEXT(
            m_cmdBuffer
            , buf.buffer
            , _offset
            , _drawCount
            , _stride
        );

        endRendering(m_cmdBuffer);

        flushWriteBarriersRec(_hPass);
    }

    VkSampler RHIContext_vk::getCachedSampler(SamplerFilter _filter, SamplerMipmapMode _mipMd, SamplerAddressMode _addrMd, SamplerReductionMode _reduMd)
    {
        KG_ZoneScopedC(Color::indian_red);

        bx::HashMurmur2A hash;
        hash.begin();
        hash.add(_filter);
        hash.add(_mipMd);
        hash.add(_addrMd);
        hash.add(_reduMd);

        uint32_t hashKey = hash.end();

        VkSampler samplerCached = m_samplerCache.find(hashKey);
        if (0 != samplerCached)
        {
            return samplerCached;
        }

        VkSampler sampler = kage::vk::createSampler(
            getFilter(_filter)
            , getSamplerMipmapMode(_mipMd)
            , getSamplerAddressMode(_addrMd)
            , getSamplerReductionMode(_reduMd)
            );

        m_samplerCache.add(hashKey, sampler);

        return sampler;
    }

    VkImageView RHIContext_vk::getCachedImageView(const ImageHandle _hImg, uint16_t _mip, uint16_t _numMips, uint16_t _numLayers, VkImageViewType _type)
    {
        KG_ZoneScopedC(Color::indian_red);

        const Image_vk& img = getImage(_hImg.id, true);

        bx::HashMurmur2A hash;
        hash.begin();
        hash.add(_hImg.id);
        hash.add(_mip);
        hash.add(_numMips);
        hash.add(_type);

        uint32_t hashKey = hash.end();

        VkImageView* viewCached = m_imgViewCache.find(hashKey);

        if (nullptr != viewCached) {
            return *viewCached;
        }

        VkImageView view = kage::vk::createImageView(
            img.image
            , img.format
            , _mip
            , _numMips
            , _numLayers
            , _type
            );

        m_imgViewCache.add(hashKey, view, _hImg.id);

        return view;
    }

    VkBufferView RHIContext_vk::getCachedBufferView(const BufferHandle _hbuf)
    {
        KG_ZoneScopedC(Color::indian_red);

        const Buffer_vk& buf = getBuffer(_hbuf.id);
        bx::HashMurmur2A hash;
        hash.begin();
        hash.add(buf.resId);

        uint32_t hashKey = hash.end();

        VkBufferView* viewCached = m_bufViewCache.find(hashKey);
        if (nullptr != viewCached) {
            return *viewCached;
        }

        VkBufferViewCreateInfo bufferViewCreateInfo = {};
        bufferViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        bufferViewCreateInfo.buffer = buf.buffer;
        bufferViewCreateInfo.format = buf.format; // Example format
        bufferViewCreateInfo.offset = 0;
        bufferViewCreateInfo.range = VK_WHOLE_SIZE;

        VkBufferView view;
        VK_CHECK(vkCreateBufferView(m_device, &bufferViewCreateInfo, nullptr, &view));

        m_bufViewCache.add(hashKey, view, _hbuf.id);

        return view;
    }

    void RHIContext_vk::createInstance()
    {
        KG_ZoneScopedC(Color::indian_red);

        m_instance = kage::vk::createInstance();
        assert(m_instance);

        volkLoadInstanceOnly(m_instance);
    }

    void RHIContext_vk::createPhysicalDevice()
    {
        VkPhysicalDevice physicalDevices[16];
        uint32_t deviceCount = sizeof(physicalDevices) / sizeof(physicalDevices[0]);
        VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices));

        m_physicalDevice = kage::vk::pickPhysicalDevice(physicalDevices, deviceCount);
        assert(m_physicalDevice);
    }

    void RHIContext_vk::uploadBuffer(const uint16_t _bufId, const void* _data, uint32_t _size)
    {
        KG_ZoneScopedC(Color::indian_red);

        assert(_size > 0);

        BufferAliasInfo bai;
        bai.size = _size;
        Buffer_vk scratch = kage::vk::createBuffer(
            bai
            , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        bx::memCopy(scratch.data, _data, _size);

        VkBufferCopy region = { 0 , 0, VkDeviceSize(_size) };

        const Buffer_vk& buffer = getBuffer(_bufId);
        
        m_barrierDispatcher.barrier(buffer.buffer,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        dispatchBarriers();

        vkCmdCopyBuffer(m_cmdBuffer, scratch.buffer, buffer.buffer, 1, &region);

        // write flush
        m_barrierDispatcher.barrier(buffer.buffer,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        dispatchBarriers();

        release(scratch.buffer);
        release(scratch.memory);
    }

    void RHIContext_vk::fillBuffer(const uint16_t _bufId, const uint32_t _value, uint32_t _size)
    {
        KG_ZoneScopedC(Color::indian_red);

        assert(_size > 0);

        const Buffer_vk& buf = getBuffer(_bufId);

        m_barrierDispatcher.barrier(buf.buffer,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        dispatchBarriers();

        vkCmdFillBuffer(m_cmdBuffer, buf.buffer, 0, _size, _value);

        // write flush
        m_barrierDispatcher.barrier(buf.buffer,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        dispatchBarriers();
    }

    uint32_t getBCBlcokSz(VkFormat _fmt)
    {
        uint32_t result = 0;
        switch (_fmt)
        {
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            result = 8;
            break;
        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
            result = 16;
            break;
        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
            result = 8;
            break;
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
            result = 16;
            break;
        default:
            message(warning, "unsupported format!!!");
            break;
        }

        return result;
    }

    uint32_t getBCImageSize(uint32_t _w, uint32_t _h, uint32_t _lv, uint32_t _blkSz)
    {
        uint32_t result = 0;
        for (uint32_t ii = 0; ii < _lv; ++ii)
        {
            result += ((_w + 3) / 4) * ((_h + 3) / 4) * _blkSz;

            _w = _w > 1 ? (_w >> 1) : 1;
            _h = _h > 1 ? (_h >> 1) : 1;
        }

        return result;
    }

    void RHIContext_vk::uploadImage(const uint16_t _imgId, const void* _data, uint32_t _size)
    {
        KG_ZoneScopedC(Color::indian_red);

        assert(_size > 0);

        
        BufferAliasInfo bai;
        bai.size = _size;
        Buffer_vk scratch = kage::vk::createBuffer(
            bai
            , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        memcpy(scratch.data, _data, _size);

        const Image_vk& vkImg = getImage(_imgId);
        const ImageCreateInfo& imgInfo = m_imgCreateInfos.getIdToData(_imgId);

        m_barrierDispatcher.barrier(
            vkImg.image
            , vkImg.aspectMask
            , { VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );

        dispatchBarriers();

        uint32_t blockSz = (imgInfo.format < ResourceFormat::undefined) ? getBCBlcokSz(vkImg.format) : 0;
        uint32_t size = (imgInfo.format < ResourceFormat::undefined) ? getBCImageSize(vkImg.width, vkImg.height, vkImg.numMips, blockSz) : _size;

        assert(size == _size);
        uint32_t bufOffset = 0;
        uint32_t w = vkImg.width;
        uint32_t h = vkImg.height;

        VkBufferImageCopy* regions = (VkBufferImageCopy*)bx::alloc(g_bxAllocator, sizeof(VkBufferImageCopy) * vkImg.numMips);
        bx::memSet(regions, 0, sizeof(VkBufferImageCopy) * vkImg.numMips);
        for (uint32_t ii = 0; ii < vkImg.numMips; ++ii)
        {
            regions[ii].bufferOffset = bufOffset;
            regions[ii].bufferRowLength = 0; // assuming tightly packed already
            regions[ii].bufferImageHeight = 0; // assuming tightly packed already

            regions[ii].imageSubresource.aspectMask = vkImg.aspectMask;
            regions[ii].imageSubresource.layerCount = vkImg.numLayers;
            regions[ii].imageSubresource.mipLevel = ii;
            regions[ii].imageSubresource.baseArrayLayer = 0; // assuming only 1 layer would use

            regions[ii].imageExtent = {w, h, 1};
            regions[ii].imageOffset = { 0, 0, 0 };

            bufOffset += ((w + 3) / 4) * ((h + 3) / 4) * blockSz;

            w = (w > 1) ? (w >> 1) : 1;
            h = (h > 1) ? (h >> 1) : 1;
        }
        vkCmdCopyBufferToImage(m_cmdBuffer, scratch.buffer, vkImg.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkImg.numMips, regions);
        
        // write flush
        m_barrierDispatcher.barrier(vkImg.image
            , vkImg.aspectMask
            , { VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );

        dispatchBarriers();
        
        bx::free(g_bxAllocator, regions);
        release(scratch.buffer);
        release(scratch.memory);
    }

    void RHIContext_vk::checkUnmatchedBarriers(uint16_t _passId)
    {
        KG_ZoneScopedC(Color::indian_red);

        bool shouldCheck = true;
        if (!shouldCheck)
            return;

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        // flush in out resources
        // write depth
        if (kInvalidHandle != passInfo.writeDepthId)
        {
            uint16_t depthId = passInfo.writeDepth.first;
            const Image_vk& depthImg = getImage(depthId);
            const Image_vk& baseImg = getImage(depthId, true);


            BarrierState_vk state = m_barrierDispatcher.getBarrierState(depthImg.image);
            BarrierState_vk baseState = m_barrierDispatcher.getBaseBarrierState(baseImg.image);

            if (baseState != state)
            {
                message(warning, "expect state mismatched for image %s, base %s"
                    , getName(ImageHandle{depthId})
                    , getName(ImageHandle{ baseImg.resId }));
                m_barrierDispatcher.barrier(depthImg.image, baseImg.aspectMask, baseState);
            }
        }

        // write colors
        for (uint32_t ii = 0; ii < passInfo.writeColors.size(); ++ii)
        {
            uint16_t id = passInfo.writeColors.getIdAt(ii);
            uint16_t baseImgId = m_aliasToBaseImages.getIdToData(id);

            const Image_vk& img = getImage(id);
            const Image_vk& baseImg = getImage(id, true);

            BarrierState_vk state = m_barrierDispatcher.getBarrierState(img.image);
            BarrierState_vk baseState = m_barrierDispatcher.getBaseBarrierState(baseImg.image);

            if (baseState != state)
            {
                message(warning, "expect state mismatched for image %s, base %s"
                    , getName(ImageHandle{ id })
                    , getName(ImageHandle{ baseImgId }));
                m_barrierDispatcher.barrier(img.image, baseImg.aspectMask, baseState);
            }
        }

        // write buffers
        for (uint32_t ii = 0; ii < passInfo.writeBuffers.size(); ++ii)
        {
            uint16_t id = passInfo.writeBuffers.getIdAt(ii);
            uint16_t baseBufId = m_aliasToBaseBuffers.getIdToData(id);

            const Buffer_vk& buf = getBuffer(id);
            const Buffer_vk& baseBuf = getBuffer(id, true);

            BarrierState_vk state = m_barrierDispatcher.getBarrierState(buf.buffer);
            BarrierState_vk baseState = m_barrierDispatcher.getBaseBarrierState(baseBuf.buffer);

            if (baseState != state)
            {
                message(warning, "expect state mismatched for buffer %s, base %s"
                    , getName(BufferHandle{ id })
                    , getName(BufferHandle{ baseBufId }));

                m_barrierDispatcher.barrier(buf.buffer, baseState);
            }
        }

        // read images
        for (uint32_t ii = 0; ii < passInfo.readImages.size(); ++ii)
        {
            uint16_t id = passInfo.readImages.getIdAt(ii);
            uint16_t baseImgId = m_aliasToBaseImages.getIdToData(id);

            const Image_vk& img = getImage(id);
            const Image_vk& baseImg = getImage(id, true);

            BarrierState_vk state = m_barrierDispatcher.getBarrierState(img.image);
            BarrierState_vk baseState = m_barrierDispatcher.getBaseBarrierState(baseImg.image);

            if (baseState != state)
            {
                message(warning, "expect state mismatched for image %s, base %s"
                    , getName(ImageHandle{ id })
                    , getName(ImageHandle{ baseImgId }));

                m_barrierDispatcher.barrier(img.image, baseImg.aspectMask, baseState);
            }
        }

        // read buffers
        for (uint32_t ii = 0; ii < passInfo.readBuffers.size(); ++ii)
        {
            uint16_t id = passInfo.readBuffers.getIdAt(ii);
            uint16_t baseBufId = m_aliasToBaseBuffers.getIdToData(id);

            const Buffer_vk& buf = getBuffer(id);
            const Buffer_vk& baseBuf = getBuffer(id, true);

            BarrierState_vk state = m_barrierDispatcher.getBarrierState(buf.buffer);
            BarrierState_vk baseState = m_barrierDispatcher.getBaseBarrierState(baseBuf.buffer);

            if (baseState != state)
            {
                message(warning, "expect state mismatched for buffer %s, base %s"
                    , getName(BufferHandle{ id })
                    , getName(BufferHandle{ baseBufId }));
                m_barrierDispatcher.barrier(buf.buffer, baseState);
            }
        }

        dispatchBarriers();
    }

    void RHIContext_vk::createBarriers(uint16_t _passId)
    {
        KG_ZoneScopedC(Color::indian_red);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        // write depth
        if (kInvalidHandle != passInfo.writeDepthId)
        {
            uint16_t depth = passInfo.writeDepth.first;

            BarrierState_vk dst = passInfo.writeDepth.second;
            const Image_vk& img = getImage(depth);
            m_barrierDispatcher.barrier(img.image, img.aspectMask, dst);
        }

        // write colors
        for (uint32_t ii = 0; ii < passInfo.writeColors.size(); ++ii)
        {
            uint16_t id = passInfo.writeColors.getIdAt(ii);

            BarrierState_vk dst = passInfo.writeColors.getIdToData(id);
            const Image_vk& img = getImage(id);
            m_barrierDispatcher.barrier(img.image, img.aspectMask, dst);
        }

        // write buffers
        for (uint32_t ii = 0; ii < passInfo.writeBuffers.size(); ++ii)
        {
            uint16_t id = passInfo.writeBuffers.getIdAt(ii);

            BarrierState_vk dst = passInfo.writeBuffers.getIdToData(id);

            m_barrierDispatcher.barrier(getBuffer(id).buffer, dst);
        }

        // read images
        for (uint32_t ii = 0; ii < passInfo.readImages.size(); ++ii)
        {
            uint16_t id = passInfo.readImages.getIdAt(ii);

            BarrierState_vk dst = passInfo.readImages.getIdToData(id);

            const Image_vk& img = getImage(id);
            m_barrierDispatcher.barrier(img.image, img.aspectMask, dst);
        }

        // read buffers
        for (uint32_t ii = 0; ii < passInfo.readBuffers.size(); ++ii)
        {
            uint16_t id = passInfo.readBuffers.getIdAt(ii);

            BarrierState_vk dst = passInfo.readBuffers.getIdToData(id);

            m_barrierDispatcher.barrier(getBuffer(id).buffer, dst);
        }
    }

    void RHIContext_vk::flushWriteBarriers(uint16_t _passId)
    {
        KG_ZoneScopedC(Color::indian_red);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        // flush in out resources
        for (uint32_t ii = 0; ii < passInfo.writeOpInToOut.size(); ++ii)
        {
            CombinedResID inId = passInfo.writeOpInToOut.getIdAt(ii);
            CombinedResID outId = passInfo.writeOpInToOut.getDataAt(ii);

            if (isBuffer(outId))
            {
                BarrierState_vk dst = passInfo.writeBuffers.getIdToData(inId.id);

                m_barrierDispatcher.barrier(getBuffer(inId.id).buffer, dst);
            }
            else if (isImage(outId) && inId.id == passInfo.writeDepthId)
            {
                uint16_t depth = passInfo.writeDepth.first;
                assert(depth == inId.id);
                
                BarrierState_vk dst = passInfo.writeDepth.second;
                
                const Image_vk& outImg = getImage(outId.id);
                m_barrierDispatcher.barrier(outImg.image, outImg.aspectMask, dst);
            }
            else if (isImage(outId) && inId.id != passInfo.writeDepthId)
            {
                BarrierState_vk dst = passInfo.writeColors.getIdToData(inId.id);

                const Image_vk& outImg = getImage(outId.id);
                m_barrierDispatcher.barrier(outImg.image, outImg.aspectMask, dst);
            }
        }

        dispatchBarriers();
    }

    void RHIContext_vk::createBarriersRec(const PassHandle _hPass)
    {
        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_hPass.id);
        for (const Binding& binding : m_descBindingSetsPerPass)
        {
            BarrierState_vk bs{};
            bs.accessMask = getBindingAccess(binding.access);
            bs.stageMask = getPipelineStageFlags(binding.stage);

            if (ResourceType::image == binding.type)
            {
                bs.imgLayout = getBindingLayout(binding.access);

                const Image_vk& img = getImage(binding.img.id);
                m_barrierDispatcher.barrier(
                    img.image
                    , img.aspectMask
                    , bs);
            }
            else if (ResourceType::buffer == binding.type)
            {
                const Buffer_vk& buf = getBuffer(binding.buf.id);
                m_barrierDispatcher.barrier(buf.buffer, bs);
            }
        }

        for (const Attachment& att : m_colorAttachPerPass)
        {
            BarrierState_vk bs{};
            bs.accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            bs.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            bs.imgLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            const Image_vk& img = getImage(att.hImg.id);
            m_barrierDispatcher.barrier(
                img.image
                , img.aspectMask
                , bs);
        }

        if (m_depthAttachPerPass.hImg != kInvalidHandle)
        {
            BarrierState_vk bs{};
            bs.accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            bs.stageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            bs.imgLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            const Image_vk& img = getImage(m_depthAttachPerPass.hImg.id);
            m_barrierDispatcher.barrier(
                img.image
                , img.aspectMask
                , bs);
        }

        dispatchBarriers();
    }

    void RHIContext_vk::flushWriteBarriersRec(const PassHandle _hPass)
    {
        stl::vector<Binding> bindings;
        bindings.reserve(m_descBindingSetsPerPass.size());
        for (const Binding& b : m_descBindingSetsPerPass)
        {
            if (b.access == BindingAccess::write 
                || b.access == BindingAccess::read_write)
            {
                bindings.push_back(b);
            }
        }

        for (const Binding& b : bindings)
        {
            BarrierState_vk bs{};
            bs.accessMask = getBindingAccess(b.access);
            bs.stageMask = getPipelineStageFlags(b.stage);

            if (ResourceType::image == b.type)
            {
                bs.imgLayout = getBindingLayout(b.access);

                const Image_vk& img = getImage(b.img.id);
                m_barrierDispatcher.barrier(
                    img.image
                    , img.aspectMask
                    , bs);
            }
            else if (ResourceType::buffer == b.type)
            {
                const Buffer_vk& buf = getBuffer(b.buf.id);
                m_barrierDispatcher.barrier(buf.buffer, bs);
            }
        }

        m_descBindingSetsPerPass.clear();
        m_colorAttachPerPass.clear();
        m_depthAttachPerPass = {};
    }

    void RHIContext_vk::lazySetDescriptorSet(const PassHandle _hPass)
    {
        if (m_descBindingSetsPerPass.empty())
        {
            return;
        }

        const uint32_t count = (uint32_t)m_descBindingSetsPerPass.size();

        stl::vector<DescriptorInfo> descInfos(count);
        for (uint32_t ii = 0; ii < (uint32_t)m_descBindingSetsPerPass.size(); ++ii)
        {
            const Binding& b = m_descBindingSetsPerPass[ii];

            DescriptorInfo di;
            if (ResourceType::image == b.type)
            {
                di = getImageDescInfo(b.img, b.mip, b.sampler);
                message(info, "set desc %02d with img: 0x%08x", ii, b.img);
            }
            else if (ResourceType::buffer == b.type)
            {
                di = getBufferDescInfo(b.buf);
                message(info, "set desc %02d with buf: 0x%08x", ii, b.buf);
            }

            descInfos[ii] = di;
        }

        PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        const Program_vk& prog = m_programContainer.getIdToData(passInfo.programId);
        vkCmdPushDescriptorSetWithTemplateKHR(m_cmdBuffer, prog.updateTemplate, prog.layout, 0, descInfos.data());
    }

    const Shader_vk& RHIContext_vk::getShader(const ShaderHandle _hShader) const
    {
        KG_ZoneScopedC(Color::indian_red);

        return m_shaderContainer.getIdToData(_hShader.id);
    }

    const Program_vk& RHIContext_vk::getProgram(const PassHandle _hPass) const
    {
        KG_ZoneScopedC(Color::indian_red);

        assert(m_passContainer.exist(_hPass.id));
        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_hPass.id);

        assert(m_programContainer.exist(passInfo.programId));
        return  m_programContainer.getIdToData(passInfo.programId);
    }

    void RHIContext_vk::beginRendering(const VkCommandBuffer& _cmdBuf, const uint16_t _passId) const
    {
        KG_ZoneScopedC(Color::indian_red);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        VkClearColorValue clearColor = { 0.f, 0.f, 0.f, 0.f};
        VkClearDepthStencilValue clearDepth = { 0.f, 0 };

        VkExtent2D extent = { 0, 0 };

        stl::vector<VkRenderingAttachmentInfo> colorAttachments(m_colorAttachPerPass.size());
        for (int ii = 0; ii < m_colorAttachPerPass.size(); ++ii)
        {
            const Attachment& att = m_colorAttachPerPass[ii];
            const Image_vk& colorTarget = getImage(att.hImg.id);

            extent.width = extent.width == 0 ? colorTarget.width : extent.width;
            extent.height = extent.height == 0 ? colorTarget.height : extent.height;

            assert(extent.width == colorTarget.width && extent.height == colorTarget.height);

            colorAttachments[ii].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            colorAttachments[ii].clearValue.color = clearColor;
            colorAttachments[ii].loadOp = getAttachmentLoadOp(att.load_op);
            colorAttachments[ii].storeOp = getAttachmentStoreOp(att.store_op);
            colorAttachments[ii].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            colorAttachments[ii].imageView = colorTarget.defaultView;
        }

        bool hasDepth = (m_depthAttachPerPass.hImg != kInvalidHandle);
        VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        if (hasDepth)
        {
            const Image_vk& depthTarget = getImage(m_depthAttachPerPass.hImg.id);

            extent.width = extent.width == 0 ? depthTarget.width : extent.width;
            extent.height = extent.height == 0 ? depthTarget.height : extent.height;

            assert(extent.width == depthTarget.width && extent.height == depthTarget.height);

            depthAttachment.clearValue.depthStencil = clearDepth;
            depthAttachment.loadOp = getAttachmentLoadOp(m_depthAttachPerPass.load_op);
            depthAttachment.storeOp = getAttachmentStoreOp(m_depthAttachPerPass.store_op);
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthAttachment.imageView = depthTarget.defaultView;
        }

        VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea.extent = extent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = (uint32_t)colorAttachments.size();
        renderingInfo.pColorAttachments = colorAttachments.data();
        renderingInfo.pDepthAttachment = hasDepth ? &depthAttachment : nullptr;

        vkCmdBeginRendering(m_cmdBuffer, &renderingInfo);
    }

    void RHIContext_vk::endRendering(const VkCommandBuffer& _cmdBuf) const
    {
        KG_ZoneScopedC(Color::indian_red);

        vkCmdEndRendering(_cmdBuf);
    }

    const DescriptorInfo RHIContext_vk::getImageDescInfo(const ImageHandle _hImg, uint16_t _mip, const SamplerHandle _hSampler)
    {
        KG_ZoneScopedC(Color::indian_red);

        VkSampler sampler = VK_NULL_HANDLE;
        if (m_samplerContainer.exist(_hSampler.id))
        {
            sampler = m_samplerContainer.getIdToData(_hSampler.id);
        }

        const Image_vk& img = getImage(_hImg.id);

        VkImageView view = VK_NULL_HANDLE;

        if (kAllMips == _mip)
        {
            view = getCachedImageView(_hImg, 0, img.numMips, img.numLayers, img.viewType);
        }
        else
        {
            view = getCachedImageView(_hImg, _mip, 1, img.numLayers, img.viewType);
        }

        VkImageLayout layout = m_barrierDispatcher.getCurrentImageLayout(img.image);

        return { sampler, view, layout };
    }

    const DescriptorInfo RHIContext_vk::getBufferDescInfo(const BufferHandle _hBuf)
    {
        KG_ZoneScopedC(Color::indian_red);

        const Buffer_vk& buf = getBuffer(_hBuf.id);

        VkBufferView view = VK_NULL_HANDLE;
        if (buf.format != VK_FORMAT_UNDEFINED) {
            view = getCachedBufferView(_hBuf); 
        }

        return { buf.buffer, view };
    }

    void RHIContext_vk::dispatchBarriers()
    {
        m_barrierDispatcher.dispatch(m_cmdBuffer);
    }

    double RHIContext_vk::getPassTime(const PassHandle _hPass)
    {
        auto it = m_passTime.find(_hPass.id);
        if (m_passTime.end() == it)
        {
            return 0.0;
        }
        return it->second;
    }

    double RHIContext_vk::getGPUTime()
    {
        return m_gpuTime;
    }

    uint64_t RHIContext_vk::getPassClipping(const PassHandle _hPass)
    {
        auto it = m_passStatistics.find(_hPass.id);
        if (m_passStatistics.end() == it)
        {
            return 0;
        }
        return it->second;
    }

    void RHIContext_vk::executePass(const uint16_t _passId)
    {
        KG_ZoneScopedC(Color::indian_red);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        if (passInfo.recorded)
        {
            execRecQueue({ _passId });
        }
        else
        {
            message(DebugMsgType::error, "not a valid execute queue! where does this pass belong?");
        }
    }
     
    bool RHIContext_vk::checkCopyableToSwapchain(const ImageHandle _hImg) const
    {
        const uint16_t baseId = m_aliasToBaseImages.getIdToData(_hImg.id);
        const ImageCreateInfo& srcInfo = m_imgCreateInfos.getIdToData(baseId);
        
        bool result = true;
        
        result &= (getFormat(srcInfo.format) == m_swapchainFormat);
        result &= (srcInfo.width == m_swapchain.m_resolution.width);
        result &= (srcInfo.height == m_swapchain.m_resolution.height);
        result &= (srcInfo.numMips == 1);
        result &= (srcInfo.numLayers == 1);
        result &= (srcInfo.layout == ImageLayout::color_attachment_optimal);
        result &= ((srcInfo.usage & ImageUsageFlagBits::color_attachment) == 1);


        return result;
    }

    bool RHIContext_vk::checkBlitableToSwapchain(const ImageHandle _hImg) const
    {
        const uint16_t baseId = m_aliasToBaseImages.getIdToData(_hImg.id);
        const ImageCreateInfo& srcInfo = m_imgCreateInfos.getIdToData(baseId);

        bool result = true;

        // based on the doc, blit image can simply understand as copy with scaling 
        // https://vulkan.lunarg.com/doc/view/1.3.243.0/windows/1.3-extensions/vkspec.html#copies-imagescaling
        // but there're a lot of conditions
        // here just a naive and simple one which cause no validation error in current demo.

        result &= (srcInfo.numMips == 1);
        result &= (srcInfo.numLayers == 1);
        result &= ((srcInfo.usage & ImageUsageFlagBits::color_attachment) == 1);

        return result;
    }

    void RHIContext_vk::drawToSwapchain(uint32_t _swapImgIdx)
    {
        if (checkCopyableToSwapchain(m_brief.presentImage))
        {
            copyToSwapchain(_swapImgIdx);
        }
        else if (checkBlitableToSwapchain(m_brief.presentImage))
        {
            blitToSwapchain(_swapImgIdx);
        }
        else
        {
            // render to swapchain
            assert(0);
        }
    }

    void RHIContext_vk::blitToSwapchain(uint32_t _swapImgIdx)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_imageContainer.exist(m_brief.presentImage.id))
        {
            message(DebugMsgType::error, "Does the presentImageId set correctly?");
            return;
        }

        // barriers
        const Image_vk& presentImg = getImage(m_brief.presentImage.id);
        m_barrierDispatcher.barrier(
            presentImg.image, presentImg.aspectMask,
            { VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );

        // swapchain image
        const VkImage& swapImg = m_swapchain.m_swapchainImages[_swapImgIdx];
        m_barrierDispatcher.barrier(
            swapImg, VK_IMAGE_ASPECT_COLOR_BIT,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );

        dispatchBarriers();

        // blit image
        uint32_t levelWidth = glm::max(1u, presentImg.width >> m_brief.presentMipLevel);
        uint32_t levelHeight = glm::max(1u, presentImg.height >> m_brief.presentMipLevel);

        VkImageBlit regions[1] = {};
        regions[0].srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        regions[0].srcSubresource.mipLevel = m_brief.presentMipLevel;
        regions[0].srcSubresource.layerCount = 1;
        regions[0].dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        regions[0].dstSubresource.layerCount = 1;

        regions[0].srcOffsets[0] = { 0, 0, 0 };
        regions[0].srcOffsets[1] = { int32_t(levelWidth), int32_t(levelHeight), 1 };
        regions[0].dstOffsets[0] = { 0, 0, 0 };
        regions[0].dstOffsets[1] = { int32_t(m_swapchain.m_resolution.width), int32_t(m_swapchain.m_resolution.height), 1 };

        vkCmdBlitImage(m_cmdBuffer, presentImg.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapImg, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, COUNTOF(regions), regions, VK_FILTER_NEAREST);

        // add present barrier
        m_barrierDispatcher.barrier(
            swapImg, VK_IMAGE_ASPECT_COLOR_BIT,
            { 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT }
        );

        dispatchBarriers();
    }

    void RHIContext_vk::copyToSwapchain(uint32_t _swapImgIdx)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (!m_imageContainer.exist(m_brief.presentImage.id))
        {
            message(DebugMsgType::error, "Does the presentImageId set correctly?");
            return;
        }
        // add swapchain barrier
        // img to present
        const Image_vk& presentImg = getImage(m_brief.presentImage.id);
        m_barrierDispatcher.barrier(
            presentImg.image
            , presentImg.aspectMask,
            { VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }

        );


        // swapchain image
        const VkImage swapImg = m_swapchain.m_swapchainImages[_swapImgIdx];
        m_barrierDispatcher.barrier(
            swapImg, VK_IMAGE_ASPECT_COLOR_BIT,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );

        dispatchBarriers();

        // copy to swapchain
        VkImageCopy copyRegion = {};
        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.extent = { m_swapchain.m_resolution.width, m_swapchain.m_resolution.height, 1 };

        vkCmdCopyImage(
            m_cmdBuffer,
            presentImg.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            swapImg, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copyRegion
        );

        // add present barrier
        m_barrierDispatcher.barrier(
            swapImg, VK_IMAGE_ASPECT_COLOR_BIT,
            { 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT }
        );

        dispatchBarriers();
    }


    template<typename Ty>
    void RHIContext_vk::release(Ty& _object)
    {
        if (VK_NULL_HANDLE != _object)
        {
            m_cmd.release(uint64_t(_object), getType<Ty>());
            _object = VK_NULL_HANDLE;
        }
    }

    void RHIContext_vk::execRecQueue(PassHandle _hPass)
    {
        CommandQueue& cq =  m_frameRecCmds.actPass(_hPass);

        bool done = false;

        const Command* cmd;
        do
        {
            cmd = cq.poll();

            if (nullptr != cmd)
            {
                switch (cmd->m_cmd)
                {
                
                case Command::record_set_constants:
                    {
                        const RecordSetConstantsCmd* rc = reinterpret_cast<const RecordSetConstantsCmd*>(cmd);
                        setConstants(_hPass, rc->m_mem);
                    }
                    break;
                case Command::record_push_descriptor_set:
                    {
                        const RecordPushDescriptorSetCmd* rc = reinterpret_cast<const RecordPushDescriptorSetCmd*>(cmd);
                        pushDescriptorSet(_hPass, rc->m_mem);
                    }
                    break;
                case Command::record_set_color_attachments:
                    {
                        const RecordSetColorAttachmentsCmd* rc = reinterpret_cast<const RecordSetColorAttachmentsCmd*>(cmd);
                        setColorAttachments(_hPass, rc->m_mem);
                    }
                    break;
                case Command::record_set_depth_attachment:
                    {
                        const RecordSetDepthAttachmentCmd* rc = reinterpret_cast<const RecordSetDepthAttachmentCmd*>(cmd);
                        setDepthAttachment(_hPass, rc->m_depthAttachment);
                    }
                    break;
                case Command::record_set_bindless:
                    {
                        const RecordSetBindingCmd* rc = reinterpret_cast<const RecordSetBindingCmd*>(cmd);
                        setBindless(_hPass, rc->m_bindless);
                    }
                    break;
                case Command::record_set_viewport:
                    {
                        const RecordSetViewportCmd* rc = reinterpret_cast<const RecordSetViewportCmd*>(cmd);
                        setViewport(_hPass, rc->m_x, rc->m_y, rc->m_w, rc->m_h);
                    }
                    break;
                case Command::record_set_scissor:
                    {
                        const RecordSetScissorCmd* rc = reinterpret_cast<const RecordSetScissorCmd*>(cmd);
                        setScissor(_hPass, rc->m_x, rc->m_y, rc->m_w, rc->m_h);
                    }
                    break;
                case Command::record_set_vertex_buffer:
                    {
                        const RecordSetVertexBufferCmd* rc = reinterpret_cast<const RecordSetVertexBufferCmd*>(cmd);
                        setVertexBuffer(_hPass, rc->m_buf);
                    }
                    break;
                case Command::record_set_index_buffer:
                    {
                        const RecordSetIndexBufferCmd* rc = reinterpret_cast<const RecordSetIndexBufferCmd*>(cmd);
                        setIndexBuffer(_hPass, rc->m_buf, rc->m_offset, rc->m_type);
                    }
                    break;
                case Command::record_blit:
                    {
                        const RecordBlitCmd* rc = reinterpret_cast<const RecordBlitCmd*>(cmd);
                        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
                    }
                    break;
                case Command::record_copy:
                    {
                        const RecordCopyCmd* rc = reinterpret_cast<const RecordCopyCmd*>(cmd);
                        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
                    }
                    break;
                case Command::record_fill_buffer:
                    {
                        const RecordFillBufferCmd* rc = reinterpret_cast<const RecordFillBufferCmd*>(cmd);
                        fillBuffer(_hPass, rc->m_buf, rc->m_val);
                    }
                    break;
                case Command::record_dispatch:
                    {
                        const RecordDispatchCmd* rc = reinterpret_cast<const RecordDispatchCmd*>(cmd);
                        dispatch(_hPass, rc->m_x, rc->m_y, rc->m_z);
                    }
                    break;
                case Command::record_dispatch_indirect:
                    {
                    const RecordDispatchIndirectCmd* rc = reinterpret_cast<const RecordDispatchIndirectCmd*>(cmd);
                    dispatchIndirect(_hPass, rc->m_buf, rc->m_off);
                    }
                    break;
                case Command::record_draw:
                    {
                        const RecordDrawCmd* rc = reinterpret_cast<const RecordDrawCmd*>(cmd);
                        draw(_hPass, rc->m_vtxCnt, rc->m_instCnt, rc->m_1stVtx, rc->m_1stInst);
                    }
                    break;
                case Command::record_draw_indirect:
                    {
                        const RecordDrawIndirectCmd* rc = reinterpret_cast<const RecordDrawIndirectCmd*>(cmd);
                        drawIndirect(_hPass, rc->m_buf, rc->m_off, rc->m_cnt);
                    }
                    break;
                case Command::record_draw_indirect_count:
                    {
                        const RecordDrawIndirectCountCmd* rc = reinterpret_cast<const RecordDrawIndirectCountCmd*>(cmd);
                        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
                    }
                    break;
                case Command::record_draw_indexed:
                    {
                        const RecordDrawIndexedCmd* rc = reinterpret_cast<const RecordDrawIndexedCmd*>(cmd);
                        drawIndexed(
                            _hPass
                            , rc->m_idxCnt
                            , rc->m_instCnt
                            , rc->m_1stIdx
                            , rc->m_vtxOff
                            , rc->m_1stInst
                        );
                    }
                    break;
                case Command::record_draw_indexed_indirect:
                    {
                        const RecordDrawIndexedIndirectCmd* rc = reinterpret_cast<const RecordDrawIndexedIndirectCmd*>(cmd);
                        drawIndexedIndirect(
                            _hPass
                            , rc->m_indirectBuf
                            , rc->m_off
                            , rc->m_cnt
                            , rc->m_stride
                        );
                    }
                    break;
                case Command::record_draw_indexed_indirect_count:
                    {
                        const RecordDrawIndexedIndirectCountCmd* rc = reinterpret_cast<const RecordDrawIndexedIndirectCountCmd*>(cmd);
                        drawIndexedIndirectCount(
                            _hPass
                            , rc->m_indirectBuf
                            , rc->m_off
                            , rc->m_cntBuf
                            , rc->m_cntOff
                            , rc->m_maxCnt
                            , rc->m_stride
                        );
                    }
                    break;
                case Command::record_draw_mesh_task:
                    {
                        const RecordDrawMeshTaskCmd* rc = reinterpret_cast<const RecordDrawMeshTaskCmd*>(cmd);
                        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
                    }
                    break;
                case Command::record_draw_mesh_task_indirect:
                    {
                        const RecordDrawMeshTaskIndirectCmd* rc = reinterpret_cast<const RecordDrawMeshTaskIndirectCmd*>(cmd);
                        drawMeshTaskIndirect(
                            _hPass
                            , rc->m_buf
                            , rc->m_off
                            , rc->m_cnt
                            , rc->m_stride
                        );
                    }
                    break;
                case Command::record_draw_mesh_task_indirect_count:
                    {
                        const RecordDrawMeshTaskIndirectCountCmd* rc = reinterpret_cast<const RecordDrawMeshTaskIndirectCountCmd*>(cmd);
                        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
                    }
                    break;
                case Command::record_end:
                    {
                        done = true;
                    }
                    break;
                default:
                    {
                        BX_ASSERT(0, "WHO ARE YOU!!! m_cmd: %d", cmd->m_cmd);
                    }
                    break;
                }
            }

        } while (cmd != nullptr && !done);
    }

    void RHIContext_vk::initTracy(VkQueue _queue, uint32_t _familyIdx)
    {
#if TRACY_ENABLE
        VkCommandPoolCreateInfo cpci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        cpci.pNext = NULL;
        cpci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cpci.queueFamilyIndex = _familyIdx;

        VK_CHECK(vkCreateCommandPool(
            m_device
            , &cpci
            , m_allocatorCb
            , &m_tracyCmdPool
        ));

        VkCommandBufferAllocateInfo cbai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        cbai.pNext = NULL;
        cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbai.commandBufferCount = 1;
        cbai.commandPool = m_tracyCmdPool;

        VK_CHECK(vkAllocateCommandBuffers(
            m_device
            , &cbai
            , &m_tracyCmdBuf
        ));

        KG_ProfVkContext(m_tracyVkCtx, m_physicalDevice, m_device, _queue, m_tracyCmdBuf);
#endif
    }

    void RHIContext_vk::destroyTracy()
    {
#if TRACY_ENABLE
        release(m_tracyCmdPool);
        KG_ProfDestroyContext(m_tracyVkCtx);

        m_tracyCmdBuf = VK_NULL_HANDLE;
#else
        ;
#endif
    }


    BarrierDispatcher::~BarrierDispatcher()
    {
        clearPending();
    }

    void BarrierDispatcher::track(
        const VkBuffer _buf
        , BarrierState_vk _state
        , const VkBuffer _baseBuf /* = VK_NULL_HANDLE */
    )
    {
        KG_ZoneScopedC(Color::indian_red);

        BX_ASSERT(m_trackingBuffers.find(_buf) == m_trackingBuffers.end(), "buffer already tracked!");

        m_trackingBuffers.insert({ _buf , { _buf, _state } });

        if (VK_NULL_HANDLE != _baseBuf)
        {
            m_aliasToBaseBuffers.insert({ _buf, _baseBuf });

            // if the buffer is the base buffer
            if (_buf == _baseBuf)
            {
                m_baseBufferStatus.insert({ _baseBuf, { _baseBuf, _state} });
            }
        }
    }

    void BarrierDispatcher::track(
        const VkImage _img
        , const VkImageAspectFlags _aspect
        , BarrierState_vk _barrierState
        , const VkImage _baseImg /* = VK_NULL_HANDLE */
    )
    {
        KG_ZoneScopedC(Color::indian_red);

        BX_ASSERT(m_trackingImages.find(_img) == m_trackingImages.end(), "image already tracked!");

        m_trackingImages.insert({ _img , { _img, _aspect, _barrierState} });

        if (VK_NULL_HANDLE != _baseImg)
        {
            m_aliasToBaseImages.insert({ _img, _baseImg });

            // if the image is the base image
            if (_img == _baseImg)
            {
                m_baseImageStatus.insert({ _baseImg, { _baseImg, _aspect, _barrierState} });
            }
        }
    }

    void BarrierDispatcher::untrack(const VkBuffer _buf)
    {
        KG_ZoneScopedC(Color::indian_red);

        // untrack the status
        {
            using Iter = decltype(m_trackingBuffers)::iterator;

            Iter it = m_trackingBuffers.find(_buf);
            if (it != m_trackingBuffers.end())
            {
                m_trackingBuffers.erase(it);
            }
        }

        // untrack the alias
        {
            using Iter = decltype(m_aliasToBaseBuffers)::iterator;

            Iter it = m_aliasToBaseBuffers.find(_buf);
            if (it != m_aliasToBaseBuffers.end())
            {
                m_aliasToBaseBuffers.erase(it);
            }
        }
    }

    void BarrierDispatcher::untrack(const VkImage _img)
    {
        KG_ZoneScopedC(Color::indian_red);

        // untrack the status
        {
            using Iter = decltype(m_trackingImages)::iterator;

            Iter it = m_trackingImages.find(_img);
            if (it != m_trackingImages.end())
            {
                m_trackingImages.erase(it);
            }
        }

        // untrack the alias
        {
            using Iter = decltype(m_aliasToBaseImages)::iterator;

            Iter it = m_aliasToBaseImages.find(_img);
            if (it != m_aliasToBaseImages.end())
            {
                m_aliasToBaseImages.erase(it);
            }
        }
    }

    bool isStageShader(VkPipelineStageFlags _stage)
    {
        return bool (
            _stage & (
                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
                | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
                | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT
                | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
                | VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT
                | VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT
                )
            );
    }

    bool isStageTransfer(VkPipelineStageFlags _stage)
    {
        return bool (
            _stage & (VK_PIPELINE_STAGE_TRANSFER_BIT)
            );
    }

    bool isStageBottom(VkPipelineStageFlags _stage)
    {
        return bool (
            _stage & (VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)
            );
    }

    bool isStageIndirect(VkPipelineStageFlags _stage)
    {
        return bool(
            _stage & (VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT)
            );
    }

    bool isStageVertexInput(VkPipelineStageFlags _stage)
    {
        return bool(
            _stage & (VK_PIPELINE_STAGE_VERTEX_INPUT_BIT)
            );
    }

    bool isStageMemory(VkPipelineStageFlags _stage)
    {
        return bool(
            _stage & (VK_PIPELINE_STAGE_HOST_BIT)
            );
    }

    bool isStageColorAttch(VkPipelineStageFlags _stage)
    {
        return bool(
            _stage & ( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT )
            );
    }

    bool isStageDepthAttch(VkPipelineStageFlags _stage)
    {
        return bool(
            _stage & ( 
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT // depth
                | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT // stencil
                )
            );
    }

    bool isAccessBinding(VkAccessFlags _access)
    {
        return bool(
            _access & ( 
                VK_ACCESS_SHADER_READ_BIT
                | VK_ACCESS_SHADER_WRITE_BIT
                )
            );
    }

    bool isAccessColorAttch(VkAccessFlags _access)
    {
        return bool(
            _access & (
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                )
            );
    }

    bool isAccessDepthAttch(VkAccessFlags _access)
    {
        return bool(
            _access & (
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                )
            );
    }

    bool isAccessTransfer(VkAccessFlags _access)
    {
        return bool(
            _access & (
                VK_ACCESS_TRANSFER_READ_BIT
                | VK_ACCESS_TRANSFER_WRITE_BIT
                )
            );
    }

    bool isAccessIndirect(VkAccessFlags _access)
    {
        return bool(
            _access & (
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT
                )
            );
    }

    bool isAccessVertexInput(VkAccessFlags _access)
    {
        return bool(
            _access & (
                VK_ACCESS_INDEX_READ_BIT
                | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
                )
            );
    }

    bool isAccessMemory(VkAccessFlags _access)
    {
        return bool(
            _access & (
                VK_ACCESS_MEMORY_READ_BIT
                | VK_ACCESS_MEMORY_WRITE_BIT
                )
            );
    }

    bool isLayoutColorAttch(VkImageLayout _layout)
    {
        return bool(
            _layout & (
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                )
            );
    }

    bool isLayoutDepthAttch(VkImageLayout _layout)
    {
        return bool(
            _layout & (
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                )
            );
    }

    bool isLayoutTransfer(VkImageLayout _layout)
    {
        return bool(
            _layout & (
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                )
            );
    }

    bool isLayoutPresent(VkImageLayout _layout)
    {
        return bool(
            _layout & (
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                )
            );
    }

    bool isLayoutShader(VkImageLayout _layout)
    {
        return bool(
            _layout & (
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                | VK_IMAGE_LAYOUT_GENERAL
                )
            );
    }


    bool validateBufferStatus(VkAccessFlags _access, VkPipelineStageFlags _stage)
    {
        bool valid = true;

        bool binding =      isAccessBinding(_access)     && isStageShader(_stage);
        bool transfer =     isAccessTransfer(_access)    && isStageTransfer(_stage);
        bool indirect =     isAccessIndirect(_access)    && isStageIndirect(_stage);
        bool vertexInput =  isAccessVertexInput(_access) && isStageVertexInput(_stage);
        bool memory =       isAccessMemory(_access)      && isStageMemory(_stage);

        valid &= (binding || transfer || indirect || vertexInput || memory);

        return valid;
    }

    bool validateImageStatus(VkAccessFlags _access, VkPipelineStageFlags _stage, VkImageLayout _layout)
    {
        bool valid = true;

        bool binding =      isAccessBinding(_access)     && isStageShader(_stage)       && isLayoutShader(_layout);
        bool colorAttch =   isAccessColorAttch(_access)  && isStageColorAttch(_stage)   && isLayoutColorAttch(_layout);
        bool depthAttch =   isAccessDepthAttch(_access)  && isStageDepthAttch(_stage)   && isLayoutDepthAttch(_layout);
        bool transfer =     isAccessTransfer(_access)    && isStageTransfer(_stage)     && isLayoutTransfer(_layout);
        bool present =                                      isStageBottom(_stage)       && isLayoutPresent(_layout);

        valid &= (binding || colorAttch || depthAttch || transfer || present);

        return valid;
    }

    void BarrierDispatcher::validate(
        const VkBuffer _buf
        , const BarrierState_vk& _state
    )
    {
        KG_ZoneScopedC(Color::indian_red);
        
        BX_ASSERT(
            m_trackingBuffers.find(_buf) != m_trackingBuffers.end()
            , "buffer: [%s] not tracking! track it first!"
            , getLocalDebugName(_buf)
        );

        BX_ASSERT(
            validateBufferStatus(_state.accessMask, _state.stageMask)
            , "buffer: [%s] status not valid! access: 0x%08x, stage: 0x%08x"
            , getLocalDebugName(_buf)
            , _state.accessMask
            , _state.stageMask
        );
    }

    void BarrierDispatcher::validate(
        const VkImage _img
        , VkImageAspectFlags _dstAspect
        , const BarrierState_vk& _dstBarrier
    )
    {
        KG_ZoneScopedC(Color::indian_red);

        BX_ASSERT(
            m_trackingImages.find(_img) != m_trackingImages.end()
            , "image: [%s] not tracking! track it first!"
            , getLocalDebugName(_img)
        );

        BX_ASSERT(
            validateImageStatus(_dstBarrier.accessMask, _dstBarrier.stageMask, _dstBarrier.imgLayout)
            , "image: [%s] status not valid! access: %s, stage: %s, layout: %s"
            , getLocalDebugName(_img)
            , getVkAccessDebugName(_dstBarrier.accessMask)
            , getVkPipelineStageDebugName(_dstBarrier.stageMask)
            , getVkImageLayoutDebugName(_dstBarrier.imgLayout)
        );
    }

    void BarrierDispatcher::barrier(const VkBuffer _buf, const BarrierState_vk& _dst)
    {
        KG_ZoneScopedC(Color::indian_red);

        BX_ASSERT(
            m_trackingBuffers.find(_buf) != m_trackingBuffers.end()
            , "buffer: %s not tracking! track it first!"
            , getLocalDebugName(_buf)
        );

        m_pendingBuffers.insert(_buf);
        
        validate(_buf, _dst);

        BarrierState_vk& bs = m_trackingBuffers[_buf].dstState;

        bs.accessMask |= _dst.accessMask;
        bs.stageMask |= _dst.stageMask;
    }

    void BarrierDispatcher::barrier(const VkImage _img, VkImageAspectFlags _dstAspect, const BarrierState_vk& _dstBarrier)
    {
        KG_ZoneScopedC(Color::indian_red);

        BX_ASSERT(
            m_trackingImages.find(_img) != m_trackingImages.end()
            , "image: %s not tracking! track it first!"
            , getLocalDebugName(_img)
        );

        m_pendingImages.insert(_img);

        validate(_img, _dstAspect, _dstBarrier);

        BarrierState_vk& src = m_trackingImages[_img].srcState;
        BarrierState_vk& dst = m_trackingImages[_img].dstState;

        dst.accessMask |= _dstBarrier.accessMask;
        dst.stageMask |= _dstBarrier.stageMask;
        dst.imgLayout = _dstBarrier.imgLayout;
    }

    void BarrierDispatcher::dispatchGlobalBarrier(const VkCommandBuffer& _cmdBuffer, const BarrierState_vk& _src, const BarrierState_vk& _dst)
    {
        VkMemoryBarrier2 ba = memoryBarrier(
            _src.accessMask
            , _dst.accessMask
            , _src.stageMask
            , _dst.stageMask
        );

        pipelineBarrier(_cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 1, &ba, 0, nullptr, 0, nullptr);
    }


    void BarrierDispatcher::dispatch(const VkCommandBuffer& _cmdBuffer)
    {
        KG_ZoneScopedC(Color::indian_red);

        if (m_pendingBuffers.empty() && m_pendingImages.empty())
        {
            return;
        }

        message(info, "- dispatch start ------------------");

        stl::vector<VkImageMemoryBarrier2> imgBarriers;
        for (const VkImage img : m_pendingImages)
        {
            const ImageStatus& st = m_trackingImages[img];
            
            const VkImageAspectFlags aspect = st.aspect;
            const BarrierState_vk src = st.srcState;
            const BarrierState_vk dst = st.dstState;

            VkImageMemoryBarrier2 barrier = imageBarrier(
                st.image
                , aspect
                , src.accessMask, src.imgLayout, src.stageMask
                , dst.accessMask, dst.imgLayout, dst.stageMask
            );

            imgBarriers.emplace_back(barrier);

            message(
                info
                , "Image Barrier: 0x%08x, %s\n\
                    aspect: 0x%08x\n\
                    , [SRC] access: 0x%08x, stage: 0x%08x, layout: 0x%016lx \n\
                    , [DST] access: 0x%08x, stage: 0x%08x, layout: 0x%016lx"
                , img
                , getLocalDebugName(img)
                , aspect
                , src.accessMask, src.stageMask, src.imgLayout
                , dst.accessMask, dst.stageMask, dst.imgLayout
            );
        }

        stl::vector<VkBufferMemoryBarrier2> bufBarriers;
        for (const VkBuffer& buf : m_pendingBuffers)
        {
            const BufferStatus& st = m_trackingBuffers[buf];

            const BarrierState_vk src = st.srcState;
            const BarrierState_vk dst = st.dstState;

            VkBufferMemoryBarrier2 barrier = bufferBarrier(
                st.buffer
                , src.accessMask, src.stageMask
                , dst.accessMask, dst.stageMask
            );

            bufBarriers.emplace_back(barrier);

            message(
                info
                , "Buffer Barrier: 0x%08x, %s\n\
                    , [SRC] access: 0x%08x, stage: 0x%08x\n\
                    , [DST] access: 0x%08x, stage: 0x%08x"
                , buf
                , getLocalDebugName(buf)
                , src.accessMask, src.stageMask
                , dst.accessMask, dst.stageMask
            );
        }

        pipelineBarrier(
            _cmdBuffer
            , VK_DEPENDENCY_BY_REGION_BIT
            , 0
            , nullptr
            , bufBarriers.size(), bufBarriers.data()
            , imgBarriers.size(), imgBarriers.data()
        );

        for (const VkImage img : m_pendingImages)
        {
            ImageStatus& st = m_trackingImages[img];

            st.srcState = st.dstState;
            st.dstState = BarrierState_vk{};

            using MapIter = decltype(m_aliasToBaseImages)::const_iterator;
            MapIter iter = m_aliasToBaseImages.find(img);
            if (iter != m_aliasToBaseImages.end())
            {
                VkImage base = iter->second;

                ImageStatus& baseSt = m_baseImageStatus[base];
                baseSt = st;
            }
        }

        for (const VkBuffer buf : m_pendingBuffers)
        {
            BufferStatus& st = m_trackingBuffers[buf];

            st.srcState = st.dstState;
            st.dstState = BarrierState_vk{};

            using MapIter = decltype(m_aliasToBaseBuffers)::const_iterator;
            MapIter iter = m_aliasToBaseBuffers.find(buf);
            if (iter != m_aliasToBaseBuffers.end())
            {
                VkBuffer base = iter->second;

                BufferStatus& baseSt = m_baseBufferStatus[base];
                baseSt = st;
            }
        }

        // clear all once barriers dispatched
        clearPending();

        message(info, "- dispatch end ------------------");
    }

    VkImageLayout BarrierDispatcher::getCurrentImageLayout(const VkImage _img) const
    {
        KG_ZoneScopedC(Color::indian_red);

        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

        using CIter = decltype(m_trackingImages)::const_iterator;

        CIter iter = m_trackingImages.find(_img);
        if (iter != m_trackingImages.end())
        {
            layout = iter->second.srcState.imgLayout;
        }

        return layout;
    }

    BarrierState_vk BarrierDispatcher::getBarrierState(const VkImage _img) const
    {
        KG_ZoneScopedC(Color::indian_red);

        BarrierState_vk result = {};

        using CIter = decltype(m_trackingImages)::const_iterator;
        CIter iter = m_trackingImages.find(_img);
        if (iter != m_trackingImages.end())
        {
            result = iter->second.dstState;
        }

        return result;
    }

    BarrierState_vk BarrierDispatcher::getBarrierState(const VkBuffer _buf) const
    {
        KG_ZoneScopedC(Color::indian_red);

        BarrierState_vk result = {};

        using CIter = decltype(m_trackingBuffers)::const_iterator;
        CIter iter = m_trackingBuffers.find(_buf);
        if (iter != m_trackingBuffers.end())
        {
            result = iter->second.dstState;
        }

        return result;
    }

    BarrierState_vk BarrierDispatcher::getBaseBarrierState(const VkImage _img) const
    {
        KG_ZoneScopedC(Color::indian_red);
        BarrierState_vk result = {};

        using MapIter = decltype(m_aliasToBaseImages)::const_iterator;
        MapIter iter = m_aliasToBaseImages.find(_img);
        if (iter != m_aliasToBaseImages.end())
        {
            const VkImage baseImg = iter->second;

            using CIter = decltype(m_baseImageStatus)::const_iterator;
            CIter baseIter = m_baseImageStatus.find(baseImg);
            if (baseIter != m_baseImageStatus.end())
            {
                result = baseIter->second.dstState;
            }
        }

        return result;
    }

    BarrierState_vk BarrierDispatcher::getBaseBarrierState(const VkBuffer _buf) const
    {
        KG_ZoneScopedC(Color::indian_red);
        BarrierState_vk result = {};

        using MapIter = decltype(m_aliasToBaseBuffers)::const_iterator;
        MapIter iter = m_aliasToBaseBuffers.find(_buf);
        if (iter != m_aliasToBaseBuffers.end())
        {
            const VkBuffer baseBuf = iter->second;

            using CIter = decltype(m_baseBufferStatus)::const_iterator;
            CIter baseIter = m_baseBufferStatus.find(baseBuf);
            if (baseIter != m_baseBufferStatus.end())
            {
                result = baseIter->second.dstState;
            }
        }

        return result;
    }

    void BarrierDispatcher::clearPending()
    {
        KG_ZoneScopedC(Color::indian_red);

        m_pendingBuffers.clear();
        m_pendingImages.clear();
    }

    void CommandQueue_vk::init(uint32_t _familyIdx, VkQueue _queue, uint32_t _numFramesInFlight)
    {
        m_queueFamilyIdx = _familyIdx;
        m_queue = _queue;
        m_numFramesInFlight = bx::clamp<uint32_t>(_numFramesInFlight, 1, kMaxNumFrameLatency);
        m_activeCommandBuffer = VK_NULL_HANDLE;

        reset();
    }

    void CommandQueue_vk::reset()
    {
        shutdown();

        m_currentFrameInFlight = 0;
        m_consumeIndex = 0;

        m_numSignalSemaphores = 0;
        m_numWaitSemaphores = 0;

        m_activeCommandBuffer = VK_NULL_HANDLE;
        m_currentFence = VK_NULL_HANDLE;
        m_completedFence = VK_NULL_HANDLE;

        m_submitted = 0;

        VkCommandPoolCreateInfo cpci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        cpci.pNext = NULL;
        cpci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        cpci.queueFamilyIndex = m_queueFamilyIdx;

        VkCommandBufferAllocateInfo cbai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        cbai.pNext = NULL;
        cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbai.commandBufferCount = 1;

        VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fci.pNext = NULL;
        fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t ii = 0; ii < m_numFramesInFlight; ++ii)
        {
            VK_CHECK(vkCreateCommandPool(
                s_renderVK->m_device
                , &cpci
                , s_renderVK->m_allocatorCb
                , &m_commandList[ii].m_commandPool
            ));

            cbai.commandPool = m_commandList[ii].m_commandPool;

            VK_CHECK( vkAllocateCommandBuffers(
                s_renderVK->m_device
                , &cbai
                , &m_commandList[ii].m_commandBuffer
            ));

            VK_CHECK(vkCreateFence(
                s_renderVK->m_device
                , &fci
                , s_renderVK->m_allocatorCb
                , &m_commandList[ii].m_fence
            ));
        }
    }

    void CommandQueue_vk::shutdown()
    {
        kick(true);
        finish(true);

        for (uint32_t ii = 0; ii < m_numFramesInFlight; ++ii)
        {
            vkDestroy(m_commandList[ii].m_fence);
            m_commandList[ii].m_commandBuffer = VK_NULL_HANDLE;
            vkDestroy(m_commandList[ii].m_commandPool);
        }
    }

    void CommandQueue_vk::alloc(VkCommandBuffer* _cmdBuf)
    {
        if (m_activeCommandBuffer == VK_NULL_HANDLE)
        {
            const VkDevice device = s_renderVK->m_device;
            CommandList& commandList = m_commandList[m_currentFrameInFlight];

            VK_CHECK(vkWaitForFences(device, 1, &commandList.m_fence, VK_TRUE, UINT64_MAX));

            VK_CHECK(vkResetCommandPool(device, commandList.m_commandPool, 0));

            VkCommandBufferBeginInfo cbi;
            cbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cbi.pNext = NULL;
            cbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            cbi.pInheritanceInfo = NULL;

            VK_CHECK(vkBeginCommandBuffer(commandList.m_commandBuffer, &cbi));

            m_activeCommandBuffer = commandList.m_commandBuffer;
            m_currentFence = commandList.m_fence;
        }

        if (NULL != _cmdBuf)
        {
            *_cmdBuf = m_activeCommandBuffer;
        }
    }

    void CommandQueue_vk::addWaitSemaphore(VkSemaphore _semaphore, VkPipelineStageFlags _stage)
    {
        BX_ASSERT(m_numWaitSemaphores < BX_COUNTOF(m_waitSemaphores), "Too many wait semaphores.");

        m_waitSemaphores[m_numWaitSemaphores] = _semaphore;
        m_waitSemaphoreStages[m_numWaitSemaphores] = _stage;
        m_numWaitSemaphores++;
    }

    void CommandQueue_vk::addSignalSemaphore(VkSemaphore _semaphore)
    {
        BX_ASSERT(m_numSignalSemaphores < BX_COUNTOF(m_signalSemaphores), "Too many signal semaphores.");

        m_signalSemaphores[m_numSignalSemaphores] = _semaphore;
        m_numSignalSemaphores++;
    }

    void CommandQueue_vk::kick(bool _wait)
    {
        if (VK_NULL_HANDLE != m_activeCommandBuffer)
        {
            const VkDevice device = s_renderVK->m_device;


            s_renderVK->m_barrierDispatcher.dispatchGlobalBarrier(
                m_activeCommandBuffer
                , { 0, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT }
                , { 0, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT }
            );

            VK_CHECK(vkEndCommandBuffer(m_activeCommandBuffer));

            m_completedFence = m_currentFence;
            m_currentFence = VK_NULL_HANDLE;

            VK_CHECK(vkResetFences(device, 1, &m_completedFence));

            VkSubmitInfo si;
            si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.pNext = NULL;
            si.waitSemaphoreCount = m_numWaitSemaphores;
            si.pWaitSemaphores = &m_waitSemaphores[0];
            si.pWaitDstStageMask = m_waitSemaphoreStages;
            si.commandBufferCount = 1;
            si.pCommandBuffers = &m_activeCommandBuffer;
            si.signalSemaphoreCount = m_numSignalSemaphores;
            si.pSignalSemaphores = &m_signalSemaphores[0];

            m_numWaitSemaphores = 0;
            m_numSignalSemaphores = 0;

            VK_CHECK(vkQueueSubmit(m_queue, 1, &si, m_completedFence));

            if (_wait)
            {
                VK_CHECK(vkWaitForFences(device, 1, &m_completedFence, VK_TRUE, UINT64_MAX));
            }

            m_activeCommandBuffer = VK_NULL_HANDLE;

            m_currentFrameInFlight = (m_currentFrameInFlight + 1) % m_numFramesInFlight;
            m_submitted++;
        }
    }

    void CommandQueue_vk::finish(bool _finishAll)
    {
        if (_finishAll)
        {
            for (uint32_t ii = 0; ii < m_numFramesInFlight; ++ii)
            {
                consume();
            }

            m_consumeIndex = m_currentFrameInFlight;
        }
        else
        {
            consume();
        }
    }

    void CommandQueue_vk::release(uint64_t _handle, VkObjectType _type)
    {
        Resource resource;
        resource.m_type = _type;
        resource.m_handle = _handle;
        m_release[m_currentFrameInFlight].push_back(resource);
    }

    void CommandQueue_vk::consume()
    {
        m_consumeIndex = (m_consumeIndex + 1) % m_numFramesInFlight;
        
        for (const Resource& resource : m_release[m_consumeIndex])
        {
            switch (resource.m_type)
            {
            case VK_OBJECT_TYPE_BUFFER:                destroy<VkBuffer             >(resource.m_handle); break;
            case VK_OBJECT_TYPE_IMAGE_VIEW:            destroy<VkImageView          >(resource.m_handle); break;
            case VK_OBJECT_TYPE_IMAGE:                 destroy<VkImage              >(resource.m_handle); break;
            case VK_OBJECT_TYPE_FRAMEBUFFER:           destroy<VkFramebuffer        >(resource.m_handle); break;
            case VK_OBJECT_TYPE_PIPELINE_LAYOUT:       destroy<VkPipelineLayout     >(resource.m_handle); break;
            case VK_OBJECT_TYPE_PIPELINE:              destroy<VkPipeline           >(resource.m_handle); break;
            case VK_OBJECT_TYPE_DESCRIPTOR_SET:        destroy<VkDescriptorSet      >(resource.m_handle); break;
            case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: destroy<VkDescriptorSetLayout>(resource.m_handle); break;
            case VK_OBJECT_TYPE_RENDER_PASS:           destroy<VkRenderPass         >(resource.m_handle); break;
            case VK_OBJECT_TYPE_SAMPLER:               destroy<VkSampler            >(resource.m_handle); break;
            case VK_OBJECT_TYPE_SEMAPHORE:             destroy<VkSemaphore          >(resource.m_handle); break;
            case VK_OBJECT_TYPE_SURFACE_KHR:           destroy<VkSurfaceKHR         >(resource.m_handle); break;
            case VK_OBJECT_TYPE_SWAPCHAIN_KHR:         destroy<VkSwapchainKHR       >(resource.m_handle); break;
            case VK_OBJECT_TYPE_DEVICE_MEMORY:         destroy<VkDeviceMemory       >(resource.m_handle); break;
            default:
                BX_ASSERT(false, "Invalid resource type: %d", resource.m_type);
                break;
            }
        }
        m_release[m_consumeIndex].clear();
    }


    void ScratchBuffer::create(uint32_t _size, uint32_t _count)
{
        const VkPhysicalDeviceMemoryProperties memProps = s_renderVK->m_memProps;
        const VkPhysicalDeviceLimits& limits = s_renderVK->m_phyDeviceProps.limits;
        const VkDevice device = s_renderVK->m_device;

        const uint32_t align = uint32_t(limits.minUniformBufferOffsetAlignment);
        const uint32_t entrySz = bx::strideAlign(_size, align);
        const uint32_t totalSz = entrySz * _count;

        BufferAliasInfo bai;
        bai.size = totalSz;
        m_buf = kage::vk::createBuffer(
            bai
            , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        reset();
    }

    bool ScratchBuffer::occupy(VkDeviceSize& _offset, const void* _data, uint32_t _size)
    {
        if (VK_NULL_HANDLE == m_buf.buffer)
        {
            return false;
        }

        if (m_offset + _size > m_buf.size)
        {
            return false;
        }

        memcpy(m_buf.data, _data, _size);

        _offset = (VkDeviceSize)m_offset;

        m_offset += _size;

        return true;
    }

    void ScratchBuffer::flush()
    {
        const VkDevice device = s_renderVK->m_device;

        kage::vk::flushBuffer(m_buf);
    }

    void ScratchBuffer::reset()
    {
        m_offset = 0;
    }

    void ScratchBuffer::destroy()
    {
        reset();

        release(m_buf.buffer);
        release(m_buf.memory);
    }

    void FrameRecCmds::init()
    {
        start();
    }

    void FrameRecCmds::record(
        const PassHandle _hPass
        , const CommandQueue& queue
        , uint32_t _offset
        , uint32_t _count
    )
    {
        RecCmdRange rec;
        rec.startIdx = m_cmdQueue.getIdx();

        m_cmdQueue.push( queue, _offset, _count );

        rec.endIdx = m_cmdQueue.getIdx();

        BX_ASSERT(rec.endIdx - rec.startIdx == _count
            , "The recorded size is mis-matched. endPos - startPos(%d) != size(%d)"
            , rec.endIdx - rec.startIdx
            , _count
        );

        m_recCmdRange.insert({ _hPass, rec });
    }

    CommandQueue& FrameRecCmds::actPass(const PassHandle _hPass)
    {
        const RecCmdRangeMap::iterator it = m_recCmdRange.find(_hPass);

        BX_ASSERT(it != m_recCmdRange.end()
            , "The pass handle is not found in the recorded commands."
        );

        const RecCmdRange& rec = it->second;

        m_cmdQueue.setIdx(rec.startIdx);

        return m_cmdQueue;
    }

    void FrameRecCmds::finish()
    {
        m_cmdQueue.finish();
    }

    void FrameRecCmds::start()
    {
        m_recCmdRange.clear();
        m_cmdQueue.start();
    }

} // namespace vk
} // namespace kage