#include "common.h"
#include "macro.h"

#include "config.h"
#include "util.h"
#include "kage_math.h"
#include "rhi_context_vk.h"

#include <algorithm> //sort


namespace kage { namespace vk
{
    RHIContext_vk* s_renderVK = nullptr;


#define VK_DESTROY_FUNC(_name)                                                          \
	void vkDestroy(Vk##_name& _obj)                                                     \
	{                                                                                   \
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
        if (VK_NULL_HANDLE != _obj)
        {
            vkDestroySurfaceKHR(s_renderVK->m_instance, _obj.vk, s_renderVK->m_allocatorCb);
            _obj = VK_NULL_HANDLE;
        }
    }

    void vkDestroy(VkDeviceMemory& _obj)
    {
        if (VK_NULL_HANDLE != _obj)
        {
            vkFreeMemory(s_renderVK->m_device, _obj.vk, s_renderVK->m_allocatorCb);
            _obj = VK_NULL_HANDLE;
        }
    }

    void vkDestroy(VkDescriptorSet& _obj)
    {
        if (VK_NULL_HANDLE != _obj)
        {
            vkFreeDescriptorSets(s_renderVK->m_device, s_renderVK->m_descPool, 1, &_obj);
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

    RHIContext* rendererCreate(const Resolution& _resolution, void* _wnd)
    {
        s_renderVK = BX_NEW(g_bxAllocator, RHIContext_vk)(g_bxAllocator);

        s_renderVK->init(_resolution, _wnd);
        
        return s_renderVK;
    }
    void rendererDestroy()
    {
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

    VkFormat getFormat(ResourceFormat _format)
    {
        VkFormat format = VK_FORMAT_UNDEFINED;

        switch (_format)
        {
        case kage::ResourceFormat::r8_sint:
            format = VK_FORMAT_R8_SINT;
            break;
        case kage::ResourceFormat::r8_uint:
            format = VK_FORMAT_R8_UINT;
            break;
        case kage::ResourceFormat::r16_uint:
            format = VK_FORMAT_R16_UINT;
            break;
        case kage::ResourceFormat::r16_sint:
            format = VK_FORMAT_R16_SINT;
            break;
        case kage::ResourceFormat::r16_snorm:
            format = VK_FORMAT_R16_SNORM;
            break;
        case kage::ResourceFormat::r16_unorm:
            format = VK_FORMAT_R16_UNORM;       
            break;
        case kage::ResourceFormat::r32_uint:
            format = VK_FORMAT_R32_UINT;
            break;
        case kage::ResourceFormat::r32_sint:
            format = VK_FORMAT_R32_SINT;
            break;
        case kage::ResourceFormat::r32_sfloat:
            format = VK_FORMAT_R32_SFLOAT;
            break;
        case kage::ResourceFormat::r32g32_uint:
            format = VK_FORMAT_R32G32_UINT;
            break;
        case kage::ResourceFormat::r32g32_sint:
            format = VK_FORMAT_R32G32_SINT;
            break;
        case kage::ResourceFormat::r32g32_sfloat:
            format = VK_FORMAT_R32G32_SFLOAT;
            break;
        case kage::ResourceFormat::r32g32b32_uint:
            format = VK_FORMAT_R32G32B32_UINT;
            break;
        case kage::ResourceFormat::r32g32b32_sint:
            format = VK_FORMAT_R32G32B32_SINT;
            break;
        case kage::ResourceFormat::r32g32b32_sfloat:
            format = VK_FORMAT_R32G32B32_SFLOAT;
            break;
        case kage::ResourceFormat::b8g8r8a8_snorm:
            format = VK_FORMAT_B8G8R8A8_SNORM;
            break;
        case kage::ResourceFormat::b8g8r8a8_unorm:
            format = VK_FORMAT_B8G8R8A8_UNORM;
            break;
        case kage::ResourceFormat::b8g8r8a8_sint:
            format = VK_FORMAT_B8G8R8A8_SINT;
            break;
        case kage::ResourceFormat::b8g8r8a8_uint:
            format = VK_FORMAT_B8G8R8A8_UINT;
            break;
        case kage::ResourceFormat::r8g8b8a8_snorm:
            format = VK_FORMAT_R8G8B8A8_SNORM;
            break;
        case kage::ResourceFormat::r8g8b8a8_unorm:
            format = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        case kage::ResourceFormat::r8g8b8a8_sint:
            format = VK_FORMAT_R8G8B8A8_SINT;
            break;
        case kage::ResourceFormat::r8g8b8a8_uint:
            format = VK_FORMAT_R8G8B8A8_UINT;
            break;
        case kage::ResourceFormat::d16:
            format = VK_FORMAT_D16_UNORM;
            break;
        case kage::ResourceFormat::d32:
            format = VK_FORMAT_D32_SFLOAT;
            break;
        default:
            format = VK_FORMAT_UNDEFINED;
            break;
        }

        return format;
    }

    VkFormat getValidFormat(ResourceFormat _format, ImageUsageFlags _usage, VkFormat _color, VkFormat _depth)
    {
        // attachments
        if (_usage &= ImageUsageFlagBits::color_attachment)
        {
            return _color;
        }
        if (_usage &= ImageUsageFlagBits::depth_stencil_attachment)
        {
            return _depth;
        }

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

    VkPipelineStageFlags getPipelineStageFlags(PipelineStageFlags _flags)
    {
        VkPipelineStageFlags flags = 0;

        if (_flags & PipelineStageFlagBits::top_of_pipe)
        {
            flags |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        if (_flags & PipelineStageFlagBits::draw_indirect)
        {
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
                //if (std::string(extension.extensionName) == extName) {
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
        props.format = getValidFormat(info.format, info.usage, _defaultColorFormat, _defaultColorFormat);
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

    RHIContext_vk::RHIContext_vk(bx::AllocatorI* _allocator)
        : RHIContext(_allocator)
        , m_nwh(nullptr)
        , m_instance{ VK_NULL_HANDLE }
        , m_device{ VK_NULL_HANDLE }
        , m_physicalDevice{ VK_NULL_HANDLE }
        , m_surface{ VK_NULL_HANDLE }
        , m_queue{ VK_NULL_HANDLE }
        , m_cmdBuffer{ VK_NULL_HANDLE }
        , m_imageFormat{ VK_FORMAT_UNDEFINED }
        , m_depthFormat{ VK_FORMAT_UNDEFINED }
        , m_gfxFamilyIdx{ VK_QUEUE_FAMILY_IGNORED }
        , m_cmdList{nullptr}
        , m_allocatorCb{nullptr}
#if _DEBUG
        , m_debugCallback{ VK_NULL_HANDLE }
#endif
    {
        VKZ_ZoneScopedC(Color::indian_red);
    }

    RHIContext_vk::~RHIContext_vk()
    {
        VKZ_ZoneScopedC(Color::indian_red);

        shutdown();

        VKZ_ProfDestroyContext(m_tracyVkCtx);
    }

    void RHIContext_vk::init(const Resolution& _resolution, void* _wnd)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        VK_CHECK(volkInitialize());

        this->createInstance();

#if _DEBUG
        m_debugCallback = registerDebugCallback(m_instance);
#endif
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

        m_swapchain.create(m_nwh);

        m_imageFormat = m_swapchain.getSwapchainFormat();
        m_depthFormat = VK_FORMAT_D32_SFLOAT;

        // fill the image to barrier
        for (uint32_t ii = 0; ii < m_swapchain.m_swapchainImageCount; ++ii)
        {
            m_barrierDispatcher.track(m_swapchain.m_swapchainImages[ii]
                , VK_IMAGE_ASPECT_COLOR_BIT
                , { 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
            );
        }

        vkGetDeviceQueue(m_device, m_gfxFamilyIdx, 0, &m_queue);
        assert(m_queue);


        {
            m_numFramesInFlight = _resolution.maxFrameLatency == 0
                ? kMaxNumFrameLatency
                : _resolution.maxFrameLatency
                ;

            m_cmd.init(m_gfxFamilyIdx, m_queue, m_numFramesInFlight);

            m_cmd.alloc(&m_cmdBuffer);

            m_cmdList = BX_NEW(m_pAllocator, CmdList_vk)(m_cmdBuffer);
        }


        {
            uint32_t descriptorCount = 512;

            VkDescriptorPoolSize poolSizes[] =
            {
                { VK_DESCRIPTOR_TYPE_SAMPLER, descriptorCount },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptorCount },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptorCount },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorCount },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorCount },
            };

            VkDescriptorPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
            poolCreateInfo.maxSets = descriptorCount;
            poolCreateInfo.poolSizeCount = COUNTOF(poolSizes);
            poolCreateInfo.pPoolSizes = poolSizes;

            VK_CHECK(vkCreateDescriptorPool(m_device, &poolCreateInfo, 0, &m_descPool));
        }

        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memProps);

        m_scratchBuffer.create();

        VKZ_ProfVkContext(m_tracyVkCtx, m_physicalDevice, m_device, m_queue, m_cmdBuffer);
    }

    void RHIContext_vk::bake()
    {
        VKZ_ZoneScopedC(Color::indian_red);

        RHIContext::bake();

        m_queryTimeStampCount = (uint32_t)(2 * m_passContainer.size());
        m_queryPoolTimeStamp = createQueryPool(m_device, m_queryTimeStampCount, VK_QUERY_TYPE_TIMESTAMP);
        assert(m_queryPoolTimeStamp);

        m_queryStatisticsCount = (uint32_t)(2 * m_passContainer.size());
        m_queryPoolStatistics = createQueryPool(m_device, m_queryStatisticsCount, VK_QUERY_TYPE_PIPELINE_STATISTICS);
        assert(m_queryPoolStatistics);
    }

    bool RHIContext_vk::render()
    {
        VKZ_ZoneScopedC(Color::indian_red);

        if (m_passContainer.size() < 1)
        {
            message(DebugMessageType::error, "no pass needs to execute in context!");
            return false;
        }

        SwapchainStatus_vk swapchainStatus = m_swapchain.getSwapchainStatus();
        if (swapchainStatus == SwapchainStatus_vk::not_ready) {  // skip this frame
            return true;
        }

        if(swapchainStatus == SwapchainStatus_vk::resize)
        {
            return true;
            // TODO: remove old swapchain images from barriers first
            assert(0);

            // fill the image to barrier
            for (uint32_t ii = 0; ii < m_swapchain.m_swapchainImageCount; ++ii)
            {
                m_barrierDispatcher.track(m_swapchain.m_swapchainImages[ii]
                    , VK_IMAGE_ASPECT_COLOR_BIT
                    , { 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
                );
            }

            recreateSwapchainImages();
        }

        m_swapchain.acquire(m_cmdBuffer);

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        // time stamp
        uint32_t queryIdx = 0;

        // zone the command buffer to fix tracy tracking issue
        {
            VKZ_VkZoneC(m_tracyVkCtx, m_cmdBuffer, "main command buffer", Color::blue);

            vkCmdResetQueryPool(m_cmdBuffer, m_queryPoolTimeStamp, 0, m_queryTimeStampCount);

            vkCmdWriteTimestamp(m_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_queryPoolTimeStamp, queryIdx);
            // render passes
            for (size_t ii = 0; ii < m_passContainer.size(); ++ii)
            {
                uint16_t passId = m_passContainer.getIdAt(ii);
                const char* pn = getName(PassHandle{ passId });

                VKZ_VkZoneTransient(m_tracyVkCtx, var, m_cmdBuffer, pn);

                //checkUnmatchedBarriers(passId);
                createBarriers(passId);
                m_barrierDispatcher.dispatch(m_cmdBuffer);

                executePass(passId);

                // will dispatch barriers internally
                flushWriteBarriers(passId);

                // write time stamp
                vkCmdWriteTimestamp(m_cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPoolTimeStamp, ++queryIdx);
            }

            // to swapchain
            drawToSwapchain(m_swapchain.m_swapchainImageIndex);
        }

        m_cmd.addWaitSemaphore(m_swapchain.m_waitSemaphore, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        m_cmd.addSignalSemaphore(m_swapchain.m_signalSemaphore);

        m_cmd.kick();
        m_cmd.alloc(&m_cmdBuffer);
        m_cmdList->update(m_cmdBuffer);

        m_swapchain.present();


        // wait
        {
            VKZ_ZoneScopedNC("wait", Color::blue);
            VK_CHECK(vkDeviceWaitIdle(m_device)); // TODO: a fence here?
        }


        m_cmd.finish();

        // set the time stamp data
        m_passTime.clear();
        stl::vector<uint64_t> timeStamps(queryIdx);
        VK_CHECK(vkGetQueryPoolResults(m_device, m_queryPoolTimeStamp, 0, (uint32_t)timeStamps.size(), sizeof(uint64_t) * timeStamps.size(), timeStamps.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT));
        assert(queryIdx == m_passContainer.size());
        for (uint32_t ii = 1; ii < queryIdx; ++ii)
        {
            uint16_t passId = m_passContainer.getIdAt(ii - 1);
            double timeStart = double(timeStamps[ii - 1]) * m_phyDeviceProps.limits.timestampPeriod * 1e-6;
            double timeEnd = double(timeStamps[ii]) * m_phyDeviceProps.limits.timestampPeriod * 1e-6;

            m_passTime.insert({ passId, timeEnd - timeStart });
        }

        return true;
    }

    void RHIContext_vk::shutdown()
    {
        VKZ_ZoneScopedC(Color::indian_red);

        if (!m_device)
        {
            return;
        }


        vkDeviceWaitIdle(m_device);

        if (m_cmdList)
        {
            bx::free(m_pAllocator, m_cmdList);
            m_cmdList = nullptr;
        }

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

        if (m_descPool)
        {
            vkDestroyDescriptorPool(m_device, m_descPool, 0);
            m_descPool = VK_NULL_HANDLE;
        }

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
        }
        m_imageContainer.clear();

        // image views
        for (uint32_t ii = 0; ii < m_imageViewContainer.size(); ++ii)
        {
            uint16_t imgViewId = m_imageViewContainer.getIdAt(ii);
            VkImageView& view = m_imageViewContainer.getDataRef(imgViewId);
            if (view)
            {
                vkDestroyImageView(m_device, view, nullptr);
                view = VK_NULL_HANDLE;
            }
        }
        m_imageViewContainer.clear();

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
            if (pass.renderFuncDataPtr)
            {
                bx::free(m_pAllocator, pass.renderFuncDataPtr);
            }
            pass.renderFunc = nullptr;

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
        m_imageViewDescContainer.clear();
        m_bufferCreateInfoContainer.clear();
        m_imageInitPropContainer.clear();

        m_programShaderIds.clear();
        m_progThreadCount.clear();

        m_aliasToBaseBuffers.clear();
        m_aliasToBaseImages.clear();
        m_imgToViewGroupIdx.clear();
        m_imgViewGroups.clear();
        m_imgIdToAliasGroupIdx.clear();
        m_imgAliasGroups.clear();
        m_bufIdToAliasGroupIdx.clear();
        m_bufAliasGroups.clear();
    }

    bool RHIContext_vk::checkSupports(VulkanSupportExtension _ext)
    {
        const char* extName = getExtName(_ext);
        stl::vector<VkExtensionProperties> supportedExtensions;
        enumrateDeviceExtPorps(m_physicalDevice, supportedExtensions);

        return checkExtSupportness(supportedExtensions, extName);
    }

    void RHIContext_vk::updateResolution(uint32_t _width, uint32_t _height)
    {
        m_resolution.width = _width;
        m_resolution.height = _height;
    }

    void RHIContext_vk::updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        if (!m_passContainer.exist(_hPass.id))
        {
            message(info, "updateThreadCount will not perform for pass %d! It might be useless after render pass sorted", _hPass.id);
            return;
        }

        assert(_threadCountX > 0);
        assert(_threadCountY > 0);
        assert(_threadCountZ > 0);

        PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        passInfo.config.threadCountX = _threadCountX;
        passInfo.config.threadCountY = _threadCountY;
        passInfo.config.threadCountZ = _threadCountZ;
    }

    void RHIContext_vk::updateBuffer(BufferHandle _hBuf, const void* _data, uint32_t _size)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        if (!m_bufferContainer.exist(_hBuf.id))
        {
            message(info, "updateBuffer will not perform for buffer %d! It might be useless after render pass sorted", _hBuf.id);
            return;
        }

        const Buffer_vk& buf = m_bufferContainer.getIdToData(_hBuf.id);
        uint16_t baseBufId = m_aliasToBaseBuffers.getIdToData(_hBuf.id);
        const Buffer_vk& baseBuf = m_bufferContainer.getIdToData(baseBufId);

        const BufferCreateInfo& createInfo = m_bufferCreateInfoContainer.getDataRef(baseBufId);
        if (!(createInfo.memFlags & MemoryPropFlagBits::host_visible))
        {
            message(info, "updateBuffer will not perform for buffer %d! Buffer must be host visible so we can map to host memory", _hBuf.id);
            return;
        }

        // re-create buffer if new size is larger than the old one
        if (buf.size != _size && baseBuf.size < _size)
        {
            m_barrierDispatcher.untrack(buf.buffer);

            stl::vector<Buffer_vk> buffers(1, buf);
            kage::vk::destroyBuffer(m_device, buffers);

            // recreate buffer
            BufferAliasInfo info{};
            info.size = _size;
            info.bufId = _hBuf.id;


            Buffer_vk newBuf = kage::vk::createBuffer(info, m_memProps, m_device, getBufferUsageFlags(createInfo.usage), getMemPropFlags(createInfo.memFlags));
            m_bufferContainer.update(_hBuf.id, newBuf);

            ResInteractDesc interact{ createInfo.barrierState };
            m_barrierDispatcher.track(
                newBuf.buffer
                , { getAccessFlags(interact.access), getPipelineStageFlags(interact.stage) }
                , newBuf.buffer
            );
        }

        const Buffer_vk& newBuf = m_bufferContainer.getIdToData(_hBuf.id);
        uploadBuffer(_hBuf.id, _data, _size);

        flushBuffer(m_device, newBuf);
    }

    void RHIContext_vk::updateCustomFuncData(const PassHandle _hPass, const void* _data, uint32_t _size)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(_data != nullptr);
        if (!m_passContainer.exist(_hPass.id))
        {
            message(info, "update will not perform for pass %d! It might be useless after render pass sorted", _hPass.id);
            return;
        }

        PassInfo_vk& passInfo = m_passContainer.getDataRef(_hPass.id);
        
        if (nullptr != passInfo.renderFuncDataPtr)
        {
            free(allocator(), passInfo.renderFuncDataPtr);
        }

        void * mem = alloc(allocator(), _size);
        memcpy(mem, _data, _size);

        passInfo.renderFuncDataPtr = mem;
        passInfo.renderFuncDataSize = _size;
    }

    void RHIContext_vk::createShader(bx::MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

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
        VKZ_ZoneScopedC(Color::indian_red);

        ProgramCreateInfo info;
        bx::read(&_reader, info, nullptr);

        stl::vector<uint16_t> shaderIds(info.shaderNum);
        bx::read(&_reader, shaderIds.data(), info.shaderNum * sizeof(uint16_t), nullptr);

        stl::vector<Shader_vk> shaders;
        for (const uint16_t sid : shaderIds)
        {
            shaders.push_back(m_shaderContainer.getIdToData(sid));
        }

        VkPipelineBindPoint bindPoint = getBindPoint(shaders);
        Program_vk prog = kage::vk::createProgram(m_device, bindPoint, shaders, info.sizePushConstants);

        m_programContainer.addOrUpdate(info.progId, prog);
        m_programShaderIds.emplace_back(shaderIds);

        assert(m_programContainer.size() == m_programShaderIds.size());
    }

    void RHIContext_vk::createPass(bx::MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

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

        // samplers
        stl::vector<uint16_t> sampleImageIds(passMeta.sampleImageNum);
        stl::vector<uint16_t> samplerIds(passMeta.sampleImageNum);
        bx::read(&_reader, sampleImageIds.data(), passMeta.sampleImageNum * sizeof(uint16_t), nullptr);
        bx::read(&_reader, samplerIds.data(), passMeta.sampleImageNum * sizeof(uint16_t), nullptr);

        // write op aliases
        stl::vector<CombinedResID> writeOpAliasInIds(passMeta.writeBufAliasNum + passMeta.writeImgAliasNum);
        bx::read(&_reader, writeOpAliasInIds.data(), (passMeta.writeBufAliasNum + passMeta.writeImgAliasNum) * sizeof(CombinedResID), nullptr);

        stl::vector<CombinedResID> writeOpAliasOutIds(passMeta.writeBufAliasNum + passMeta.writeImgAliasNum);
        bx::read(&_reader, writeOpAliasOutIds.data(), (passMeta.writeBufAliasNum + passMeta.writeImgAliasNum) * sizeof(CombinedResID), nullptr);


        VkPipeline pipeline{};
        if (passMeta.queue == PassExeQueue::graphics)
        {
            // create pipeline
            const uint16_t progIdx = (uint16_t)m_programContainer.getIdIndex(passMeta.programId);
            const Program_vk& program = m_programContainer.getIdToData(passMeta.programId);
            const stl::vector<uint16_t>& shaderIds = m_programShaderIds[progIdx];

            uint32_t depthNum = (passMeta.writeDepthId == kInvalidHandle) ? 0 : 1;

            VkPipelineRenderingCreateInfo renderInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
            renderInfo.colorAttachmentCount = passMeta.writeImageNum - depthNum;
            renderInfo.pColorAttachmentFormats = &m_imageFormat;
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

        // fill pass info
        PassInfo_vk passInfo{};
        passInfo.passId = passMeta.passId;
        passInfo.pipeline = pipeline;

        passInfo.vertexBufferId = passMeta.vertexBufferId;
        passInfo.vertexCount = passMeta.vertexCount;
        passInfo.indexBufferId = passMeta.indexBufferId;
        passInfo.indexCount = passMeta.indexCount;

        passInfo.indirectBufferId = passMeta.indirectBufferId;
        passInfo.indirectBufOffset = passMeta.indirectBufOffset;
        passInfo.indirectBufStride = passMeta.indirectBufStride;
        passInfo.indirectCountBufferId = passMeta.indirectCountBufferId;
        passInfo.indirectCountBufOffset = passMeta.indirectCountBufOffset;
        passInfo.indirectMaxDrawCount = passMeta.indirectMaxDrawCount;

        passInfo.writeDepthId = passMeta.writeDepthId;

        passInfo.renderFunc = passMeta.renderFunc;
        passInfo.renderFuncDataPtr = passMeta.renderFuncDataPtr;
        passInfo.renderFuncDataSize = passMeta.renderFuncDataSize;

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
        passInfo.config = passMeta.passConfig;


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
            , stl::vector<std::pair<uint32_t, CombinedResID>>& _bindings
            , const ResourceType _type
            ) {
                for (uint32_t ii = 0; ii < _ids.size(); ++ii)
                {
                    _container.addOrUpdate(_ids[ii], getBarrierState(interacts[offset + ii]));

                    const uint32_t bindPoint = interacts[offset + ii].binding;

                    if (kDescriptorSetBindingDontCare == bindPoint) {
                        continue;
                    }

                    assert(kInvalidDescriptorSetIndex != bindPoint);

                    auto it = std::find_if(_bindings.begin(), _bindings.end(), [bindPoint](const std::pair<uint32_t, CombinedResID>& pair) {
                        return pair.first == bindPoint;
                        });
                    
                    if (it == _bindings.end())
                    {
                        CombinedResID combined{ _ids[ii], _type };
                        _bindings.push_back({ bindPoint, combined });
                    }
                    else
                    {
                        // same binding already exist because manually in vkz.cpp, should handle it in barrier creation
                        //assert(0);
                    }
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

        preparePassBarriers(writeImageIds, passInfo.writeColors, passInfo.bindingToColorIds, ResourceType::image);

        preparePassBarriers(readImageIds, passInfo.readImages, passInfo.bindingToResIds, ResourceType::image);
        preparePassBarriers(readBufferIds, passInfo.readBuffers, passInfo.bindingToResIds, ResourceType::buffer);
        preparePassBarriers(writeBufferIds, passInfo.writeBuffers, passInfo.bindingToResIds, ResourceType::buffer);
        assert(offset == interacts.size());

        // sort bindings
        auto sortFunc2 = [](const std::pair<uint32_t, CombinedResID>& _lhs, const std::pair<uint32_t, CombinedResID>& _rhs) -> bool {
            return _lhs.first < _rhs.first;
            };
        std::sort(passInfo.bindingToResIds.begin(), passInfo.bindingToResIds.end(), sortFunc2);
        std::sort(passInfo.bindingToColorIds.begin(), passInfo.bindingToColorIds.end(), sortFunc2);

        // samplers
        for (uint16_t ii = 0; ii < passMeta.sampleImageNum; ++ii)
        {
            passInfo.imageToSamplerIds.addOrUpdate(sampleImageIds[ii], samplerIds[ii]);
        }

        // push constants
        if (passInfo.queue == PassExeQueue::graphics
            || passInfo.queue == PassExeQueue::compute)
        {
            const Program_vk& prog = m_programContainer.getIdToData(passMeta.programId);
            m_constantsMemBlock.addConstant({ passInfo.passId }, prog.pushConstantSize);
        }

        // one-operation passes
        if (passInfo.queue == PassExeQueue::fill_buffer)
        {
            assert(writeBufferIds.size() == 1);
            passInfo.oneOpWriteRes = { writeBufferIds[0], ResourceType::buffer };
        }

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

        m_passContainer.addOrUpdate(passInfo.passId, passInfo);
    }

    void RHIContext_vk::createImage(bx::MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        ImageCreateInfo info;
        bx::read(&_reader, info, nullptr);

        ImageAliasInfo* resArr = new ImageAliasInfo[info.resCount];
        bx::read(&_reader, resArr, sizeof(ImageAliasInfo) * info.resCount, nullptr);

        // so the res handle should map to the real buffer array
        stl::vector<ImageAliasInfo> infoList(resArr, resArr + info.resCount);

        ImgInitProps_vk initPorps = getImageInitProp(info, m_imageFormat, m_depthFormat);

        stl::vector<Image_vk> images;
        kage::vk::createImage(images, infoList, m_device, m_memProps, initPorps);
        assert(images.size() == info.resCount);

        m_imageInitPropContainer.addOrUpdate(info.imgId, info);

        for (int ii = 0; ii < info.resCount; ++ii)
        {
            m_imageContainer.addOrUpdate(resArr[ii].imgId, images[ii]);
            m_aliasToBaseImages.addOrUpdate(resArr[ii].imgId, info.imgId);
            m_imgIdToAliasGroupIdx.insert({ resArr[ii].imgId, (uint32_t)m_imgAliasGroups.size() });
        }
        m_imgAliasGroups.emplace_back(infoList);


        const Image_vk& baseImage = getImage(info.imgId, true);
        for (uint32_t ii = 0; ii < info.viewCount; ++ii)
        {
            const ImageViewHandle imgView = info.mipViews[ii];
            const ImageViewDesc& viewDesc = m_imageViewDescContainer.getIdToData(imgView.id);

            VkImageView view_vk = kage::vk::createImageView(m_device, baseImage.image, baseImage.format, viewDesc.baseMip, viewDesc.mipLevels);
            m_imageViewContainer.addOrUpdate(imgView.id, view_vk);
        }

        m_imgToViewGroupIdx.insert({ info.imgId, (uint16_t)m_imgViewGroups.size() });

        stl::vector<ImageViewHandle> viewHandles(info.mipViews, info.mipViews + info.viewCount);
        m_imgViewGroups.push_back(viewHandles);

        for (int ii = 0; ii < info.resCount; ++ii)
        {
            ResInteractDesc interact{ info.barrierState };

            m_barrierDispatcher.track(images[ii].image
                , getImageAspectFlags(images[ii].aspectMask)
                , { getAccessFlags(interact.access), getImageLayout(interact.layout) ,getPipelineStageFlags(interact.stage) }
                , baseImage.image
            );
        }

        if (info.pData != nullptr)
        {
            uploadImage(info.imgId, info.pData, info.size);
        }
        VKZ_DELETE_ARRAY(resArr);
    }

    void RHIContext_vk::createBuffer(bx::MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        BufferCreateInfo info;
        bx::read(&_reader, info, nullptr);

        BufferAliasInfo* resArr = new BufferAliasInfo[info.resCount];
        bx::read(&_reader, resArr, sizeof(BufferAliasInfo) * info.resCount, nullptr);

        // so the res handle should map to the real buffer array
        stl::vector<BufferAliasInfo> infoList(resArr, resArr + info.resCount);

        stl::vector<Buffer_vk> buffers;
        kage::vk::createBuffer(buffers, infoList, m_memProps, m_device, getBufferUsageFlags(info.usage), getMemPropFlags(info.memFlags));

        assert(buffers.size() == info.resCount);

        for (int ii = 0; ii < info.resCount; ++ii)
        {
            buffers[ii].fillVal = info.fillVal;
            m_bufferContainer.addOrUpdate(resArr[ii].bufId, buffers[ii]);
            m_aliasToBaseBuffers.addOrUpdate(resArr[ii].bufId, info.bufId);

            m_bufIdToAliasGroupIdx.insert({ resArr[ii].bufId, (uint16_t)m_bufAliasGroups.size() });
        }
        m_bufAliasGroups.emplace_back(infoList);

        m_bufferCreateInfoContainer.addOrUpdate(info.bufId, info);

        const Buffer_vk& baseBuffer = getBuffer(info.bufId);
        for (int ii = 0; ii < info.resCount; ++ii)
        {
            ResInteractDesc interact{ info.barrierState };
            m_barrierDispatcher.track(buffers[ii].buffer
                , { getAccessFlags(interact.access), getPipelineStageFlags(interact.stage) }
                , baseBuffer.buffer
            );
        }

        VKZ_DELETE_ARRAY(resArr);
        
        // initialize buffer
        if (info.pData != nullptr)
        {
            uploadBuffer(info.bufId, info.pData, info.size);
        }
        else
        {
            fillBuffer(info.bufId, info.fillVal, info.size);
        }
    }

    void RHIContext_vk::createSampler(bx::MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        SamplerMetaData meta;
        bx::read(&_reader, meta, nullptr);

        VkSampler sampler = kage::vk::createSampler(m_device, getSamplerReductionMode( meta.reductionMode));
        assert(sampler);

        m_samplerContainer.addOrUpdate(meta.samplerId, sampler);
    }

    void RHIContext_vk::createImageView(bx::MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        ImageViewDesc desc;
        bx::read(&_reader, desc, nullptr);

        // only store the descriptor, will do the real creation during image creation
        m_imageViewDescContainer.addOrUpdate(desc.imgViewId, desc);
    }

    void RHIContext_vk::setBackBuffers(bx::MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        uint32_t count;
        bx::read(&_reader, count, nullptr);

        stl::vector<uint16_t> ids(count);
        bx::read(&_reader, ids.data(), count * sizeof(uint16_t), nullptr);

        for (uint16_t id : ids)
        {
            m_swapchainImageIds.insert(id);
        }
    }

    void RHIContext_vk::setBrief(bx::MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        RHIBrief brief;
        bx::read(&_reader, brief, nullptr);

        m_brief = brief;
    }

    void RHIContext_vk::createInstance()
    {
        VKZ_ZoneScopedC(Color::indian_red);

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

    void RHIContext_vk::recreateSwapchainImages()
    {
        // destroy images
        for (uint16_t id : m_swapchainImageIds)
        {
            uint32_t aliasGroupIdx = m_imgIdToAliasGroupIdx.find(id)->second;
            const stl::vector<ImageAliasInfo>& aliases = m_imgAliasGroups[aliasGroupIdx];

            stl::vector<Image_vk> images;
            for (ImageAliasInfo aliasInfo : aliases)
            {
                images.push_back(m_imageContainer.getIdToData(aliasInfo.imgId));
            }

            // remove barrier tracking
            for (const Image_vk& img : images)
            {
                m_barrierDispatcher.untrack(img.image);
            }

            // destroy Image
            kage::vk::destroyImage(m_device, images);
            
            // destroy image views
            uint16_t baseId = m_aliasToBaseImages.getIdToData(id);
            auto it = m_imgToViewGroupIdx.find(baseId);
            if (it != m_imgToViewGroupIdx.end())
            {
                const stl::vector<ImageViewHandle>& viewHandles = m_imgViewGroups[it->second];
                for (ImageViewHandle view : viewHandles)
                {
                    vkDestroyImageView(m_device, m_imageViewContainer.getIdToData(view.id), nullptr);
                }
            }
        }
        // create images
        for (uint16_t id : m_swapchainImageIds)
        {
            uint32_t aliasGroupIdx = m_imgIdToAliasGroupIdx.find(id)->second;
            const stl::vector<ImageAliasInfo>& aliases = m_imgAliasGroups[aliasGroupIdx];

            uint16_t baseId = m_aliasToBaseImages.getIdToData(id);
            ImageCreateInfo& createInfo = m_imageInitPropContainer.getDataRef(baseId);
            createInfo.width = m_swapchain.m_resolution.width;
            createInfo.height = m_swapchain.m_resolution.height;

            ImgInitProps_vk initPorps = getImageInitProp(createInfo, m_imageFormat, m_depthFormat);

            //image
            stl::vector<Image_vk> images;
            kage::vk::createImage(images, aliases, m_device, m_memProps, initPorps);

            for (uint16_t ii = 0 ; ii < aliases.size(); ++ii)
            {
                m_imageContainer.update(aliases[ii].imgId, images[ii]);
            }

            // views
            const Image_vk& baseImg = m_imageContainer.getIdToData(baseId);
            auto it = m_imgToViewGroupIdx.find(baseId);
            if (it != m_imgToViewGroupIdx.end())
            {
                const stl::vector<ImageViewHandle>& viewHandles = m_imgViewGroups[it->second];

                for (ImageViewHandle view : viewHandles)
                {
                    const ImageViewDesc& viewDesc = m_imageViewDescContainer.getIdToData(view.id);
                    VkImageView view_vk = kage::vk::createImageView(m_device, baseImg.image, baseImg.format, viewDesc.baseMip, viewDesc.mipLevels);
                    m_imageViewContainer.update(view.id, view_vk);
                }
            }

            for (int ii = 0; ii < aliases.size(); ++ii)
            {
                ResInteractDesc interact{ createInfo.barrierState };

                m_barrierDispatcher.track(images[ii].image
                    , getImageAspectFlags(images[ii].aspectMask)
                    , { getAccessFlags(interact.access), getImageLayout(interact.layout) ,getPipelineStageFlags(interact.stage) }
                    , baseImg.image
                );
            }
        }
    }

    void RHIContext_vk::uploadBuffer(const uint16_t _bufId, const void* _data, uint32_t _size)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(_size > 0);


        BufferAliasInfo bai;
        bai.size = _size;
        Buffer_vk scratch = kage::vk::createBuffer(
            bai
            , m_memProps
            , m_device
            , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );


        memcpy(scratch.data, _data, _size);

        VkBufferCopy region = { 0 , 0, VkDeviceSize(_size) };

        const Buffer_vk& buffer = getBuffer(_bufId);
        
        m_barrierDispatcher.barrier(buffer.buffer,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        m_barrierDispatcher.dispatch(m_cmdBuffer);

        vkCmdCopyBuffer(m_cmdBuffer, scratch.buffer, buffer.buffer, 1, &region);

        // write flush
        m_barrierDispatcher.barrier(buffer.buffer,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        m_barrierDispatcher.dispatch(m_cmdBuffer);

        release(scratch.buffer);
        release(scratch.memory);
    }

    void RHIContext_vk::fillBuffer(const uint16_t _bufId, const uint32_t _value, uint32_t _size)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(_size > 0);

        const Buffer_vk& buf = getBuffer(_bufId);

        m_barrierDispatcher.barrier(buf.buffer,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        m_barrierDispatcher.dispatch(m_cmdBuffer);

        vkCmdFillBuffer(m_cmdBuffer, buf.buffer, 0, _size, _value);

        // write flush
        m_barrierDispatcher.barrier(buf.buffer,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        m_barrierDispatcher.dispatch(m_cmdBuffer);
    }

    void RHIContext_vk::uploadImage(const uint16_t _imgId, const void* _data, uint32_t _size)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(_size > 0);

        
        BufferAliasInfo bai;
        bai.size = _size;
        Buffer_vk scratch = kage::vk::createBuffer(
            bai
            , m_memProps
            , m_device
            , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );


        memcpy(scratch.data, _data, _size);


        const Image_vk& vkImg = getImage(_imgId);
        const ImageCreateInfo& imgInfo = m_imageInitPropContainer.getIdToData(_imgId);

        m_barrierDispatcher.barrier(vkImg.image
            , vkImg.aspectMask
            , { VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );

        m_barrierDispatcher.dispatch(m_cmdBuffer);

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.imageSubresource.aspectMask = vkImg.aspectMask;
        region.imageSubresource.layerCount = vkImg.numLayers;
        region.imageExtent.width = vkImg.width;
        region.imageExtent.height = vkImg.height;
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(m_cmdBuffer, scratch.buffer, vkImg.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        // write flush
        m_barrierDispatcher.barrier(vkImg.image
            , vkImg.aspectMask
            , { VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );

        message(info, "release buffer: 0x%llx", scratch.buffer);
        
        release(scratch.buffer);
        release(scratch.memory);
    }

    void RHIContext_vk::checkUnmatchedBarriers(uint16_t _passId)
    {
        VKZ_ZoneScopedC(Color::indian_red);

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
                message(warning, "expect state mismatched for image %d, base %d", depthId, baseImg);
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
                message(warning, "expect state mismatched for image %d, base %d", id, baseImgId);
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
                message(warning, "expect state mismatched for buffer %d, base %d", id, baseBufId);
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
                message(warning, "expect state mismatched for image %d, base %d", id, baseImgId);
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
                message(warning, "expect state mismatched for buffer %d, base %d", id, baseBufId);
                m_barrierDispatcher.barrier(buf.buffer, baseState);
            }
        }

        message(info, "+++++++ mismatched barriers +++++++++++++++++++++");
        m_barrierDispatcher.dispatch(m_cmdBuffer);
    }

    void RHIContext_vk::createBarriers(uint16_t _passId)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        // write depth
        if (kInvalidHandle != passInfo.writeDepthId)
        {
            uint16_t depth = passInfo.writeDepth.first;

            uint16_t baseDepth = m_aliasToBaseImages.getIdToData(depth);

            BarrierState_vk dst = passInfo.writeDepth.second;

            const Image_vk& img = getImage(depth);
            m_barrierDispatcher.barrier(img.image, img.aspectMask, dst);
        }

        // write colors
        for (uint32_t ii = 0; ii < passInfo.writeColors.size(); ++ii)
        {
            uint16_t id = passInfo.writeColors.getIdAt(ii);
            uint16_t baseImg = m_aliasToBaseImages.getIdToData(id);

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
        VKZ_ZoneScopedC(Color::indian_red);

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

        //message(info, "========= after pass %d ======================================", _passId);
        m_barrierDispatcher.dispatch(m_cmdBuffer);
    }

    void RHIContext_vk::pushDescriptorSetWithTemplates(const uint16_t _passId)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        pushDescriptorSetWithTemplates(m_cmdBuffer, { _passId });
    }

    void RHIContext_vk::pushDescriptorSetWithTemplates(const VkCommandBuffer& _cmdBuf, const uint16_t _passId) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        stl::vector<std::pair<uint32_t, CombinedResID>> bindingToDescInfo{};
        bindingToDescInfo.insert(end(bindingToDescInfo), begin(passInfo.bindingToResIds), end(passInfo.bindingToResIds));

        if (passInfo.queue == PassExeQueue::compute) {
            bindingToDescInfo.insert(end(bindingToDescInfo), begin(passInfo.bindingToColorIds), end(passInfo.bindingToColorIds));
        }

        auto sortFunc = [](const std::pair<uint32_t, CombinedResID>& _lhs, const std::pair<uint32_t, CombinedResID>& _rhs) -> bool {
            return _lhs.first < _rhs.first;
            };
        std::sort(begin(bindingToDescInfo), end(bindingToDescInfo), sortFunc);


        stl::vector<DescriptorInfo> descInfos(bindingToDescInfo.size());
        for (int ii = 0; ii < bindingToDescInfo.size(); ++ii)
        {
            assert(bindingToDescInfo[ii].first == ii);

            const CombinedResID resId = bindingToDescInfo[ii].second;

            if (isImage(resId))
            {
                const Image_vk& img = getImage(resId.id);

                VkSampler sampler = 0;
                if (passInfo.imageToSamplerIds.exist(resId.id))
                {
                    const uint16_t samplerId = passInfo.imageToSamplerIds.getIdToData(resId.id);
                    sampler = m_samplerContainer.getIdToData(samplerId);
                }

                VkImageView view = img.defaultView;
                VkImageLayout layout = m_barrierDispatcher.getDstImageLayout(img.image);
                DescriptorInfo info{ sampler, view, layout };
                descInfos[ii] = info;
            }
            else if (isBuffer(resId))
            {
                const Buffer_vk& buf = getBuffer(resId.id);

                DescriptorInfo info{ buf.buffer };
                descInfos[ii] = info;
            }
            else
            {
                message(DebugMessageType::error, "not a valid resource type!");
            }
        }
        if (!descInfos.empty())
        {
            const Program_vk& prog = m_programContainer.getIdToData(passInfo.programId);
            vkCmdPushDescriptorSetWithTemplateKHR(_cmdBuf, prog.updateTemplate, prog.layout, 0, descInfos.data());
        }
    }

    const Shader_vk& RHIContext_vk::getShader(const ShaderHandle _hShader) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        return m_shaderContainer.getIdToData(_hShader.id);
    }

    const Program_vk& RHIContext_vk::getProgram(const PassHandle _hPass) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(m_passContainer.exist(_hPass.id));
        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_hPass.id);

        assert(m_programContainer.exist(passInfo.programId));
        return  m_programContainer.getIdToData(passInfo.programId);
    }

    void RHIContext_vk::beginRendering(const VkCommandBuffer& _cmdBuf, const uint16_t _passId) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        VkClearColorValue color = { 33.f / 255.f, 200.f / 255.f, 242.f / 255.f, 1 };
        VkClearDepthStencilValue depth = { 0.f, 0 };

        stl::vector<VkRenderingAttachmentInfo> colorAttachments(passInfo.writeColors.size());

        assert(passInfo.bindingToColorIds.size() == passInfo.writeColors.size());
        for (int ii = 0; ii < passInfo.writeColors.size(); ++ii)
        {
            const CombinedResID imgId = passInfo.bindingToColorIds[ii].second;
            const Image_vk& colorTarget = getImage(imgId.id);

            colorAttachments[ii].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            colorAttachments[ii].clearValue.color = color;
            colorAttachments[ii].loadOp = getAttachmentLoadOp(passInfo.config.colorLoadOp);
            colorAttachments[ii].storeOp = getAttachmentStoreOp(passInfo.config.colorStoreOp);
            colorAttachments[ii].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            colorAttachments[ii].imageView = colorTarget.defaultView;
        }

        bool hasDepth = (passInfo.writeDepthId != kInvalidHandle);
        VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        if (hasDepth)
        {
            const Image_vk& depthTarget = getImage(passInfo.writeDepthId);

            depthAttachment.clearValue.depthStencil = depth;
            depthAttachment.loadOp = getAttachmentLoadOp(passInfo.config.depthLoadOp);
            depthAttachment.storeOp = getAttachmentStoreOp(passInfo.config.depthStoreOp);
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            depthAttachment.imageView = depthTarget.defaultView;
        }

        VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea.extent.width = m_swapchain.m_resolution.width;
        renderingInfo.renderArea.extent.height = m_swapchain.m_resolution.height;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = (uint32_t)colorAttachments.size();
        renderingInfo.pColorAttachments = colorAttachments.data();
        renderingInfo.pDepthAttachment = hasDepth ? &depthAttachment : nullptr;

        vkCmdBeginRendering(m_cmdBuffer, &renderingInfo);
    }

    void RHIContext_vk::endRendering(const VkCommandBuffer& _cmdBuf) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        vkCmdEndRendering(_cmdBuf);
    }

    const DescriptorInfo RHIContext_vk::getImageDescInfo(const ImageHandle _hImg, const ImageViewHandle _hImgView, const SamplerHandle _hSampler) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        VkSampler sampler = nullptr;
        if (m_samplerContainer.exist(_hSampler.id))
        {
            sampler = m_samplerContainer.getIdToData(_hSampler.id);
        }
        
        const Image_vk& img = getImage(_hImg.id);

        VkImageView view = img.defaultView;
        if (m_imageViewContainer.exist(_hImgView.id))
        {
            view = m_imageViewContainer.getIdToData(_hImgView.id);
        }
        
        VkImageLayout layout = m_barrierDispatcher.getDstImageLayout(img.image);
        
        return { sampler, view, layout };
    }

    const DescriptorInfo RHIContext_vk::getBufferDescInfo(const BufferHandle _hBuf) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        const Buffer_vk& buf = getBuffer(_hBuf.id);

        return { buf.buffer };
    }

    void RHIContext_vk::barrier(BufferHandle _hBuf, AccessFlags _access, PipelineStageFlags _stage)
    {
        const Buffer_vk& buf = getBuffer(_hBuf.id);
        m_barrierDispatcher.barrier(
            buf.buffer
            , { getAccessFlags(_access), getPipelineStageFlags(_stage) }
        );
    }

    void RHIContext_vk::barrier(ImageHandle _hImg, AccessFlags _access, ImageLayout _layout, PipelineStageFlags _stage)
    {
        const Image_vk& img = getImage(_hImg.id);
        m_barrierDispatcher.barrier(
            img.image
            , img.aspectMask
            , { getAccessFlags(_access), getImageLayout(_layout), getPipelineStageFlags(_stage) }
        );
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

    void RHIContext_vk::pushConstants(const uint16_t _passId)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);
        const Program_vk& prog = m_programContainer.getIdToData(passInfo.programId);

        uint32_t size = m_constantsMemBlock.getConstantSize({ _passId });
        const void* pData = m_constantsMemBlock.getConstantData({ _passId });

        if (size > 0 && pData != nullptr)
        {
            vkCmdPushConstants(m_cmdBuffer, prog.layout, prog.pushConstantStages, 0, size, pData);
        }
    }

    void RHIContext_vk::executePass(const uint16_t _passId)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        if (passInfo.renderFunc != nullptr)
        {
            if (PassExeQueue::graphics == passInfo.queue || PassExeQueue::compute == passInfo.queue)
            {
                const Program_vk& prog = m_programContainer.getIdToData(passInfo.programId);
                vkCmdBindPipeline(m_cmdBuffer, prog.bindPoint, passInfo.pipeline);
            }
 
            passInfo.renderFunc(*m_cmdList, passInfo.renderFuncDataPtr, passInfo.renderFuncDataSize);
            return;
        }

        if (PassExeQueue::graphics == passInfo.queue)
        {
            exeGraphic(_passId);
        }
        else if (PassExeQueue::compute == passInfo.queue)
        {
            exeCompute(_passId);
        }
        else if (PassExeQueue::copy == passInfo.queue)
        {
            exeCopy(_passId);
        }
        else if (PassExeQueue::fill_buffer == passInfo.queue)
        {
            exeFillBuffer(_passId);
        }
        else
        {
            message(DebugMessageType::error, "not a valid execute queue! where does this pass belong?");
        }
    }

    void RHIContext_vk::exeGraphic(const uint16_t _passId)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        beginRendering(m_cmdBuffer, _passId);

        // drawcalls
        const uint32_t width = m_swapchain.m_resolution.width;
        const uint32_t height = m_swapchain.m_resolution.height;

        VkViewport viewport = { 0.f, float(height), float(width), -float(height), 0.f, 1.f };
        VkRect2D scissor = { {0, 0}, {width, height } };

        vkCmdSetViewport(m_cmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(m_cmdBuffer, 0, 1, &scissor);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);
        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passInfo.pipeline);

        pushDescriptorSetWithTemplates(_passId);

        pushConstants(_passId);

        // set vertex buffer
        if (kInvalidHandle != passInfo.vertexBufferId)
        {
            VkDeviceSize offsets[1] = { 0 };
            const Buffer_vk& vertexBuffer =  getBuffer(passInfo.vertexBufferId);
            vkCmdBindVertexBuffers(m_cmdBuffer, 0, 1, &vertexBuffer.buffer, offsets);
        }

        // set index buffer
        if (kInvalidHandle != passInfo.indexBufferId)
        {
            const Buffer_vk& indexBuffer = getBuffer(passInfo.indexBufferId);
            vkCmdBindIndexBuffer(m_cmdBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32); // TODO: do I need to expose the index type out?
        }

        if (kInvalidHandle != passInfo.indirectCountBufferId && kInvalidHandle != passInfo.indirectBufferId)
        {
            const Buffer_vk& indirectCountBuffer = getBuffer(passInfo.indirectCountBufferId);
            const Buffer_vk& indirectBuffer = getBuffer(passInfo.indirectBufferId);

            vkCmdDrawIndexedIndirectCount(m_cmdBuffer
                , indirectBuffer.buffer
                , passInfo.indirectBufOffset
                , indirectCountBuffer.buffer
                , passInfo.indirectCountBufOffset
                , passInfo.indirectMaxDrawCount
                , passInfo.indirectBufStride);
        }
        else if (kInvalidHandle != passInfo.indirectBufferId)
        {
            const Buffer_vk& indirectBuffer = getBuffer(passInfo.indirectBufferId);

            vkCmdDrawIndexedIndirect(m_cmdBuffer
                , indirectBuffer.buffer
                , passInfo.indirectBufOffset
                , passInfo.indirectMaxDrawCount
                , passInfo.indirectBufStride);
        }
        else if (kInvalidHandle != passInfo.indirectCountBufferId)
        {
            const Buffer_vk& indirectCountBuffer = getBuffer(passInfo.indirectCountBufferId);

            vkCmdDrawIndirectCount(m_cmdBuffer
                           , indirectCountBuffer.buffer
                           , passInfo.indirectCountBufOffset
                           , indirectCountBuffer.buffer
                           , passInfo.indirectCountBufOffset
                           , passInfo.indirectMaxDrawCount
                           , passInfo.indirectBufStride);
        }
        else if (kInvalidHandle != passInfo.indirectBufferId)
        {
            const Buffer_vk& indirectBuffer = getBuffer(passInfo.indirectBufferId);

            vkCmdDrawIndirect(m_cmdBuffer
                           , indirectBuffer.buffer
                           , passInfo.indirectBufOffset
                           , passInfo.indirectMaxDrawCount
                           , passInfo.indirectBufStride);
        }
        else if (kInvalidHandle != passInfo.indexBufferId && passInfo.indexCount != 0)
        {
            vkCmdDrawIndexed(m_cmdBuffer, passInfo.indexCount, 1, 0, 0, 0);
        }
        else if (kInvalidHandle != passInfo.vertexBufferId && passInfo.vertexCount != 0)
        {
            vkCmdDraw(m_cmdBuffer, passInfo.vertexCount, 1, 0, 0);
        }
        else
        {
            // not supported
            assert(0);
        }

        endRendering(m_cmdBuffer);
    }

    void RHIContext_vk::exeCompute(const uint16_t _passId)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, passInfo.pipeline);
        
        pushConstants(_passId);
        pushDescriptorSetWithTemplates(_passId);

        // get the dispatch size
        const uint16_t progIdx = (uint16_t)m_programContainer.getIdIndex(passInfo.programId);
        const stl::vector<uint16_t>& shaderIds = m_programShaderIds[progIdx];
        assert(shaderIds.size() == 1);
        const Shader_vk& shader = m_shaderContainer.getIdToData(shaderIds[0]);
        
        vkCmdDispatch( m_cmdBuffer
            , calcGroupCount(passInfo.config.threadCountX, shader.localSizeX)
            , calcGroupCount(passInfo.config.threadCountY, shader.localSizeY)
            , calcGroupCount(passInfo.config.threadCountZ, shader.localSizeZ)
        );
    }

    void RHIContext_vk::exeCopy(const uint16_t _passId)
    {
        assert(0);
    }

    void RHIContext_vk::exeBlit(const uint16_t _passId)
    {
        assert(0);
    }

    void RHIContext_vk::exeFillBuffer(const uint16_t _passId)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);
        CombinedResID res = passInfo.oneOpWriteRes;
        const Buffer_vk& buf = getBuffer(res.id);

        vkCmdFillBuffer(m_cmdBuffer, buf.buffer, 0, buf.size, buf.fillVal);
    }
     
    bool RHIContext_vk::checkCopyableToSwapchain(const uint16_t _imgId) const
    {
        const uint16_t baseId = m_aliasToBaseImages.getIdToData(_imgId);
        const ImageCreateInfo& srcInfo = m_imageInitPropContainer.getIdToData(baseId);
        
        bool result = true;
        
        result &= (getFormat(srcInfo.format) == m_imageFormat);
        result &= (srcInfo.width == m_swapchain.m_resolution.width);
        result &= (srcInfo.height == m_swapchain.m_resolution.height);
        result &= (srcInfo.numMips == 1);
        result &= (srcInfo.numLayers == 1);
        result &= (srcInfo.layout == ImageLayout::color_attachment_optimal);
        result &= ((srcInfo.usage & ImageUsageFlagBits::color_attachment) == 1);


        return result;
    }

    bool RHIContext_vk::checkBlitableToSwapchain(const uint16_t _imgId) const
    {
        const uint16_t baseId = m_aliasToBaseImages.getIdToData(_imgId);
        const ImageCreateInfo& srcInfo = m_imageInitPropContainer.getIdToData(baseId);

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
        if (checkCopyableToSwapchain(m_brief.presentImageId))
        {
            copyToSwapchain(_swapImgIdx);
        }
        else if (checkBlitableToSwapchain(m_brief.presentImageId))
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
        VKZ_ZoneScopedC(Color::indian_red);

        if (!m_imageContainer.exist(m_brief.presentImageId))
        {
            message(DebugMessageType::error, "Does the presentImageId set correctly?");
            return;
        }

        // barriers
        const Image_vk& presentImg = getImage(m_brief.presentImageId);
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

        m_barrierDispatcher.dispatch(m_cmdBuffer);

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

        m_barrierDispatcher.dispatch(m_cmdBuffer);
    }

    void RHIContext_vk::copyToSwapchain(uint32_t _swapImgIdx)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        if (!m_imageContainer.exist(m_brief.presentImageId))
        {
            message(DebugMessageType::error, "Does the presentImageId set correctly?");
            return;
        }
        // add swapchain barrier
        // img to present
        const Image_vk& presentImg = getImage(m_brief.presentImageId);

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

        m_barrierDispatcher.dispatch(m_cmdBuffer);

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

        m_barrierDispatcher.dispatch(m_cmdBuffer);
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


    void BarrierDispatcher::track(const VkImage _img, const ImageAspectFlags _aspect, BarrierState_vk _barrierState, const VkImage _baseImg/* = 0*/)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(m_imgAspectFlags.find(_img) == m_imgAspectFlags.end());
        assert(m_imgBarrierStatus.find(_img) == m_imgBarrierStatus.end());

        m_imgAspectFlags.insert({ _img, getImageAspectFlags(_aspect) });
        m_imgBarrierStatus.insert({ _img , _barrierState });

        if (_baseImg != 0) {

            m_aliasToBaseImages.insert({ _img, _baseImg });
            if (_img == _baseImg)
            {
                m_baseImgAspectFlags.insert({ _baseImg, getImageAspectFlags(_aspect) });
                m_baseImgBarrierStatus.insert({ _baseImg , _barrierState });
            }
        }
    }

    void BarrierDispatcher::untrack(const VkBuffer _buf)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        {
            auto it = m_aliasToBaseBuffers.find(_buf);
            m_aliasToBaseBuffers.erase(it);
        }

        {
            auto it = m_bufBarrierStatus.find(_buf);
            m_bufBarrierStatus.erase(it);
        }
    }

    void BarrierDispatcher::untrack(const VkImage _img)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        {
            auto it = m_aliasToBaseImages.find(_img);
            m_aliasToBaseImages.erase(it);
        }

        {
            auto it = m_imgAspectFlags.find(_img);
            m_imgAspectFlags.erase(it);
        }

        {
            auto it = m_imgBarrierStatus.find(_img);
            m_imgBarrierStatus.erase(it);
        }
    }

    BarrierDispatcher::~BarrierDispatcher()
    {
        clear();
    }

    void BarrierDispatcher::track(const VkBuffer _buf, BarrierState_vk _barrierState, const VkBuffer _baseBuf/* = 0*/)
    {
        VKZ_ZoneScopedC(Color::indian_red);
        
        assert(m_bufBarrierStatus.find(_buf) == m_bufBarrierStatus.end());

        m_bufBarrierStatus.insert({ _buf , _barrierState });

        if (_baseBuf != 0) {
            m_aliasToBaseBuffers.insert({ _buf, _baseBuf });

            if (_buf == _baseBuf)
            {
                m_baseBufBarrierStatus.insert({ _baseBuf , _barrierState });
            }
        }
    }

    void BarrierDispatcher::barrier(const VkBuffer _buf, const BarrierState_vk& _dst)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        auto pair = m_bufBarrierStatus.find(_buf);
        assert(pair != m_bufBarrierStatus.end());

        const size_t idx = getElemIndex(m_dispatchBuffers, _buf);
        if (kInvalidHandle == idx)
        {
            m_dispatchBuffers.emplace_back(_buf);

            const BarrierState_vk src = pair->second;
            m_srcBufBarriers.emplace_back(src);

            m_dstBufBarriers.emplace_back(BarrierState_vk{ _dst.accessMask , _dst.stageMask });
        }
        else
        {
            BarrierState_vk& dst = m_dstBufBarriers[idx];
            dst.accessMask |= _dst.accessMask;
            dst.stageMask |= _dst.stageMask;
        }
    }

    void BarrierDispatcher::barrier(const VkImage _img, VkImageAspectFlags _aspect, const BarrierState_vk& _dst)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(m_imgAspectFlags.find(_img) != m_imgAspectFlags.end());

        auto barrierPair = m_imgBarrierStatus.find(_img);
        assert(barrierPair != m_imgBarrierStatus.end());

        const size_t idx = getElemIndex(m_dispatchImages, _img);

        if (kInvalidHandle == idx)
        {
            m_dispatchImages.emplace_back(_img);

            m_imgAspects.emplace_back(_aspect);

            BarrierState_vk src = barrierPair->second;
            m_srcImgBarriers.emplace_back(src);

            m_dstImgBarriers.push_back({ _dst.accessMask, _dst.imgLayout, _dst.stageMask });
        }
        else
        {
            assert(_aspect == m_imgAspects[idx]);

            BarrierState_vk& dst = m_dstImgBarriers[idx];
            dst.accessMask |= _dst.accessMask;
            dst.stageMask |= _dst.stageMask;
            dst.imgLayout = _dst.imgLayout; // use the new destinate layout instead
        }
    }

    void BarrierDispatcher::dispatchGlobalBarrier(const VkCommandBuffer& _cmdBuffer, const BarrierState_vk& _src, const BarrierState_vk& _dst)
    {
        return;
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
        VKZ_ZoneScopedC(Color::indian_red);

        if(m_dispatchBuffers.empty() && m_dispatchImages.empty())
        {
            return;
        }

        stl::vector<VkImageMemoryBarrier2> imgBarriers;
        for (uint32_t ii = 0; ii < m_dispatchImages.size(); ++ii)
        {
            const VkImage& img = m_dispatchImages[ii];
            const VkImageAspectFlags aspect = getImageAspectFlags(m_imgAspects[ii]);
            BarrierState_vk src = m_srcImgBarriers[ii];
            BarrierState_vk dst = m_dstImgBarriers[ii];

            VkImageMemoryBarrier2 barrier = imageBarrier(
                img, aspect,
                src.accessMask, src.imgLayout, src.stageMask,
                dst.accessMask, dst.imgLayout, dst.stageMask);
            
            imgBarriers.emplace_back(barrier);

            {
                auto& aspectPair = m_imgAspectFlags.find(img);
                aspectPair->second = aspect;
            }

            {
                auto& barrierPair = m_imgBarrierStatus.find(img);
                barrierPair->second = dst;
            }

            auto baseImgPair = m_aliasToBaseImages.find(img);
            if (baseImgPair != m_aliasToBaseImages.end())
            {
                const VkImage baseImg = baseImgPair->second;

                {
                    auto& aspectPair = m_baseImgAspectFlags.find(img);
                    aspectPair->second = aspect;
                }

                {
                    auto& barrierPair = m_baseImgBarrierStatus.find(img);
                    barrierPair->second = dst;
                }
            }
        }

        stl::vector<VkBufferMemoryBarrier2> bufBarriers;
        for (uint32_t ii = 0; ii < m_dispatchBuffers.size(); ++ii)
        {
            const VkBuffer& buf = m_dispatchBuffers[ii];
            BarrierState_vk src = m_srcBufBarriers[ii];
            BarrierState_vk dst = m_dstBufBarriers[ii];

            VkBufferMemoryBarrier2 barrier = bufferBarrier(
                buf,
                src.accessMask, src.stageMask,
                dst.accessMask, dst.stageMask);

            bufBarriers.emplace_back(barrier);

            {
                auto& barrierPair = m_bufBarrierStatus.find(buf);
                barrierPair->second = dst;
            }

            auto baseBufPair = m_aliasToBaseBuffers.find(buf);
            if(baseBufPair != m_aliasToBaseBuffers.end())
            {
                const VkBuffer baseBuf = baseBufPair->second;
                
                {
                    auto& barrierPair = m_baseBufBarrierStatus.find(buf);
                    barrierPair->second = dst;
                }
            }
        }
        
        pipelineBarrier(
            _cmdBuffer
            , VK_DEPENDENCY_BY_REGION_BIT
            , 0, nullptr
            , bufBarriers.size(), bufBarriers.data()
            , imgBarriers.size(), imgBarriers.data()
        );
        
        // clear all once barriers dispatched
        clear();
    }

    const VkImageLayout BarrierDispatcher::getDstImageLayout(const VkImage _img) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        const size_t idx = getElemIndex(m_dispatchImages, _img);
        if (kInvalidIndex != idx)
        {
            return m_dstImgBarriers[idx].imgLayout;
        }

        auto barrierPair = m_imgBarrierStatus.find(_img);
        if (barrierPair != m_imgBarrierStatus.end())
        {
            return barrierPair->second.imgLayout;
        }

        assert(0);
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }

    const BarrierState_vk BarrierDispatcher::getBarrierState(const VkImage _img) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        auto barrierPair = m_imgBarrierStatus.find(_img);
        if (barrierPair != m_imgBarrierStatus.end())
        {
            return barrierPair->second;
        }
        
        assert(0);
        return BarrierState_vk{};
    }

    const BarrierState_vk BarrierDispatcher::getBarrierState(const VkBuffer _buf) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        auto barrierPair = m_bufBarrierStatus.find(_buf);
        if (barrierPair != m_bufBarrierStatus.end())
        {
            return barrierPair->second;
        }

        return BarrierState_vk{};
    }

    const BarrierState_vk BarrierDispatcher::getBaseBarrierState(const VkImage _img) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        auto barrierPair = m_baseImgBarrierStatus.find(_img);
        if (barrierPair != m_baseImgBarrierStatus.end())
        {
            return barrierPair->second;
        }

        return BarrierState_vk{};
    }

    const BarrierState_vk BarrierDispatcher::getBaseBarrierState(const VkBuffer _buf) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        auto barrierPair = m_baseBufBarrierStatus.find(_buf);
        if (barrierPair != m_baseBufBarrierStatus.end())
        {
            return barrierPair->second;
        }

        return BarrierState_vk{};
    }

    void BarrierDispatcher::clear()
    {
        VKZ_ZoneScopedC(Color::indian_red);

        m_dispatchImages.clear();
        m_imgAspects.clear();
        m_srcImgBarriers.clear();
        m_dstImgBarriers.clear();
        
        m_dispatchBuffers.clear();
        m_srcBufBarriers.clear();
        m_dstBufBarriers.clear();
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

        VkCommandPoolCreateInfo cpci;
        cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cpci.pNext = NULL;
        cpci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        cpci.queueFamilyIndex = m_queueFamilyIdx;

        VkCommandBufferAllocateInfo cbai;
        cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbai.pNext = NULL;
        cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbai.commandBufferCount = 1;

        VkFenceCreateInfo fci;
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
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


    void ScractchBuffer::create()
    {
        const VkPhysicalDeviceMemoryProperties memProps = s_renderVK->m_memProps;
        const VkDevice device = s_renderVK->m_device;

        BufferAliasInfo bai;
        bai.size = kScratchBufferSize;
        m_buffer = kage::vk::createBuffer(
            bai
            , memProps
            , device
            , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        m_size = bai.size;

        reset();
    }

    bool ScractchBuffer::occupy(VkDeviceSize& _offset, const void* _data, uint32_t _size)
    {
        if (VK_NULL_HANDLE == m_buffer.buffer)
        {
            return false;
        }

        if (m_offset + _size > m_size)
        {
            return false;
        }

        memcpy(m_buffer.data, _data, _size);

        _offset = (VkDeviceSize)m_offset;

        m_offset += _size;

        return true;
    }

    void ScractchBuffer::reset()
    {
        m_offset = 0;
    }

    void ScractchBuffer::destroy()
    {
        reset();

        release(m_buffer.buffer);
        release(m_buffer.memory);
    }

} // namespace vk
} // namespace kage