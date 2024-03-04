#include "common.h"
#include "macro.h"

#include "config.h"
#include "util.h"

#include "rhi_context_vk.h"

#include <algorithm> //sort


namespace vkz
{
    const char* getExtName(VulkanSupportExtension _ext)
    {
        switch (_ext)
        {
        case vkz::VulkanSupportExtension::ext_swapchain:
            return VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        case vkz::VulkanSupportExtension::ext_push_descriptor:
            return VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME;
        case vkz::VulkanSupportExtension::ext_8bit_storage:
            return VK_KHR_8BIT_STORAGE_EXTENSION_NAME;
        case vkz::VulkanSupportExtension::ext_16bit_storage:
            return VK_KHR_16BIT_STORAGE_EXTENSION_NAME;
        case vkz::VulkanSupportExtension::ext_mesh_shader:
            return VK_EXT_MESH_SHADER_EXTENSION_NAME;
        case vkz::VulkanSupportExtension::ext_spirv_1_4:
            return VK_KHR_SPIRV_1_4_EXTENSION_NAME;
        case vkz::VulkanSupportExtension::ext_shader_float_controls:
            return VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME;
        case vkz::VulkanSupportExtension::ext_shader_draw_parameters:
            return VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME;
        case vkz::VulkanSupportExtension::ext_draw_indriect_count:
            return VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME;
        case vkz::VulkanSupportExtension::ext_shader_float16_int8:
            return VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME;
        case vkz::VulkanSupportExtension::ext_fragment_shading_rate:
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
        case vkz::ResourceFormat::r32g32_uint:
            format = VK_FORMAT_R32G32_UINT;
            break;
        case vkz::ResourceFormat::r32g32_sint:
            format = VK_FORMAT_R32G32_SINT;
            break;
        case vkz::ResourceFormat::r32g32_sfloat:
            format = VK_FORMAT_R32G32_SFLOAT;
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

    VkFormat getValidFormat(ResourceFormat _format, ImageUsageFlags _usage, VkFormat _color, VkFormat _depth)
    {
        if (_usage &= ImageUsageFlagBits::color_attachment)
        {
            return _color;
        }
        else if (_usage &= ImageUsageFlagBits::depth_stencil_attachment)
        {
            return _depth;
        }
        else
        {
            return getFormat(_format);
        }
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
        VkAttachmentLoadOp op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

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
            op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            break;
        }

        return op;
    }

    VkAttachmentStoreOp getAttachmentStoreOp(AttachmentStoreOp _op)
    {
        VkAttachmentStoreOp op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        switch (_op)
        {
        case AttachmentStoreOp::store:
            op = VK_ATTACHMENT_STORE_OP_STORE;
            break;
        case AttachmentStoreOp::dont_care:
            op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            break;
        default:
            op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
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

        const vkz::Memory* vtxBindingMem = vkz::alloc(uint32_t(sizeof(VkVertexInputBindingDescription) * bindings.size()));
        memcpy(vtxBindingMem->data, bindings.data(), sizeof(VkVertexInputAttributeDescription) * bindings.size());

        const vkz::Memory* vtxAttributeMem = vkz::alloc(uint32_t(sizeof(vkz::VertexAttributeDesc) * attributes.size()));
        memcpy(vtxAttributeMem->data, attributes.data(), sizeof(vkz::VertexAttributeDesc) * attributes.size());

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
            vkz::message(vkz::info, "%s : %s\n", extName, extSupported ? "true" : "false");
        }


        return extSupported;
    }

    VkSemaphore createSemaphore(VkDevice device)
    {

        VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkSemaphore semaphore = 0;
        VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &semaphore));

        return semaphore;
    }

    VkCommandPool createCommandPool(VkDevice device, uint32_t familyIndex)
    {

        VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = familyIndex;

        VkCommandPool cmdPool = 0;
        VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &cmdPool));

        return cmdPool;
    }

    ImgInitProps_vk getImageInitProp(const ImageCreateInfo& info, VkFormat _defaultColorFormat, VkFormat _defaultDepthFormat)
    {
        ImgInitProps_vk props{};
        props.level = info.mipLevels;
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

    RHIContext_vk::RHIContext_vk(AllocatorI* _allocator, RHI_Config _config, void* _wnd)
        : RHIContext(_allocator)
        , m_instance{ VK_NULL_HANDLE }
        , m_device{ VK_NULL_HANDLE }
        , m_phyDevice{ VK_NULL_HANDLE }
        , m_surface{ VK_NULL_HANDLE }
        , m_queue{ VK_NULL_HANDLE }
        , m_cmdPool{ VK_NULL_HANDLE }
        , m_cmdBuffer{ VK_NULL_HANDLE }
        , m_acquirSemaphore{ VK_NULL_HANDLE }
        , m_releaseSemaphore{ VK_NULL_HANDLE }
        , m_imageFormat{ VK_FORMAT_UNDEFINED }
        , m_depthFormat{ VK_FORMAT_UNDEFINED }
        , m_pWindow{ nullptr }
        , m_swapchain{}
        , m_gfxFamilyIdx{ VK_QUEUE_FAMILY_IGNORED }
        , m_cmdList{nullptr}
#if _DEBUG
        , m_debugCallback{ VK_NULL_HANDLE }
#endif
    {
        VKZ_ZoneScopedC(Color::indian_red);
        init(_config, _wnd);
    }

    RHIContext_vk::~RHIContext_vk()
    {
        VKZ_ZoneScopedC(Color::indian_red);
    }

    void RHIContext_vk::init(RHI_Config _config, void* _wnd)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        VK_CHECK(volkInitialize());

        this->createInstance();

#if _DEBUG
        m_debugCallback = registerDebugCallback(m_instance);
#endif
        createPhysicalDevice();

        stl::vector<VkExtensionProperties> supportedExtensions;
        enumrateDeviceExtPorps(m_phyDevice, supportedExtensions);

        m_supportMeshShading = checkExtSupportness(supportedExtensions, VK_EXT_MESH_SHADER_EXTENSION_NAME, false);


        vkGetPhysicalDeviceProperties(m_phyDevice, &m_phyDeviceProps);
        assert(m_phyDeviceProps.limits.timestampPeriod);

        m_gfxFamilyIdx = getGraphicsFamilyIndex(m_phyDevice);
        assert(m_gfxFamilyIdx != VK_QUEUE_FAMILY_IGNORED);

        m_device = vkz::createDevice(m_instance, m_phyDevice, m_gfxFamilyIdx, m_supportMeshShading);
        assert(m_device);
        
        // only single device used in this application.
        volkLoadDevice(m_device);

        m_surface = createSurface(m_instance, _wnd);
        assert(m_surface);

        VkBool32 presentSupported = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(m_phyDevice, m_gfxFamilyIdx, m_surface, &presentSupported));
        assert(presentSupported);

        m_acquirSemaphore = createSemaphore(m_device);
        assert(m_acquirSemaphore);

        m_releaseSemaphore = createSemaphore(m_device);
        assert(m_releaseSemaphore);

        m_imageFormat = vkz::getSwapchainFormat(m_phyDevice, m_surface);
        m_depthFormat = VK_FORMAT_D32_SFLOAT;

        vkz::createSwapchain(m_swapchain, m_phyDevice, m_device, m_surface, m_gfxFamilyIdx, m_imageFormat);

        // fill the image to barrier
        for (uint32_t ii = 0; ii < m_swapchain.imageCount; ++ii)
        {
            m_barrierDispatcher.addImage(m_swapchain.images[ii]
                , VK_IMAGE_ASPECT_COLOR_BIT
                , { 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
            );
        }

        vkGetDeviceQueue(m_device, m_gfxFamilyIdx, 0, &m_queue);
        assert(m_queue);


        m_cmdPool = createCommandPool(m_device, m_gfxFamilyIdx);
        assert(m_cmdPool);

        VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool = m_cmdPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        VK_CHECK(vkAllocateCommandBuffers(m_device, &allocateInfo, &m_cmdBuffer));

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

        vkGetPhysicalDeviceMemoryProperties(m_phyDevice, &m_memProps);


        BufferAliasInfo scratchAlias;
        scratchAlias.size = 128 * 1024 * 1024; // 128M
        m_scratchBuffer = vkz::createBuffer(scratchAlias, m_memProps, m_device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        m_cmdList = VKZ_NEW(m_pAllocator, CmdList_vk(m_cmdBuffer, this));
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

        SwapchainStatus_vk swapchainStatus = resizeSwapchainIfNecessary(m_swapchain, m_phyDevice, m_device, m_surface, m_gfxFamilyIdx, m_imageFormat);
        if (swapchainStatus == SwapchainStatus_vk::not_ready) {  // skip this frame
            return true;
        }

        if(swapchainStatus == SwapchainStatus_vk::resize)
        {
            // TODO: remove old swapchain images from barriers first
            assert(0);

            // fill the image to barrier
            for (uint32_t ii = 0; ii < m_swapchain.imageCount; ++ii)
            {
                m_barrierDispatcher.addImage(m_swapchain.images[ii]
                    , VK_IMAGE_ASPECT_COLOR_BIT
                    , { 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
                );
            }

            recreateBackBuffers();
        }

        uint32_t imageIndex = 0;
        VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain.swapchain, ~0ull, m_acquirSemaphore, VK_NULL_HANDLE, &imageIndex));

        VK_CHECK(vkResetCommandPool(m_device, m_cmdPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(m_cmdBuffer, &beginInfo));

        // time stamp
        uint32_t queryIdx = 0;
        vkCmdResetQueryPool(m_cmdBuffer, m_queryPoolTimeStamp, 0, m_queryTimeStampCount);
        

        vkCmdWriteTimestamp(m_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_queryPoolTimeStamp, queryIdx);
        // render passes
        for (size_t ii = 0; ii < m_passContainer.size(); ++ii)
        {
            uint16_t passId = m_passContainer.getIdAt(ii);
            //checkUnmatchedBarriers(passId);
            createBarriers(passId);
            //message(info, "---------- before pass %d ------------------------------", passId);
            m_barrierDispatcher.dispatch(m_cmdBuffer);

            executePass(passId);

            // will dispatch barriers internally
            flushWriteBarriers(passId); 

            // write time stamp
            vkCmdWriteTimestamp(m_cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPoolTimeStamp, ++queryIdx);
        }

        // copy to swapchain
        copyToSwapchain(imageIndex);

        VK_CHECK(vkEndCommandBuffer(m_cmdBuffer));

        // submit
        {
            VKZ_ZoneScopedC(Color::green);
            VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

            VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &m_acquirSemaphore;
            submitInfo.pWaitDstStageMask = &submitStageMask;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &m_cmdBuffer;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &m_releaseSemaphore;

            VK_CHECK(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));

            VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &m_swapchain.swapchain;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.pWaitSemaphores = &m_releaseSemaphore;
            presentInfo.waitSemaphoreCount = 1;

            VK_CHECK(vkQueuePresentKHR(m_queue, &presentInfo));
        }

        {
            VKZ_ZoneScopedC(Color::blue);
            VK_CHECK(vkDeviceWaitIdle(m_device)); // TODO: a fence here?
        }

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

            m_passTime.emplace({ passId, timeEnd - timeStart });
        }

        return true;
    }

    bool RHIContext_vk::checkSupports(VulkanSupportExtension _ext)
    {
        const char* extName = getExtName(_ext);
        stl::vector<VkExtensionProperties> supportedExtensions;
        enumrateDeviceExtPorps(m_phyDevice, supportedExtensions);

        return checkExtSupportness(supportedExtensions, extName);
    }

    void RHIContext_vk::resizeBackbuffers(uint32_t _width, uint32_t _height)
    {
        m_backBufferWidth = _width;
        m_backBufferHeight = _height;
    }

    void RHIContext_vk::updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ)
    {
        VKZ_ZoneScopedC(Color::indian_red);

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

        const Buffer_vk& buf = m_bufferContainer.getIdToData(_hBuf.id);
        uint16_t baseBufId = m_aliasToBaseBuffers.getIdToData(_hBuf.id);
        const Buffer_vk& baseBuf = m_bufferContainer.getIdToData(baseBufId);

        // re-create buffer if new size is larger than the old one
        if (buf.size != _size && baseBuf.size < _size)
        {
            m_barrierDispatcher.removeBuffer(buf.buffer);

            stl::vector<Buffer_vk> buffers(1, buf);
            vkz::destroyBuffer(m_device, buffers);

            // recreate buffer
            BufferAliasInfo info{};
            info.size = _size;
            info.bufId = _hBuf.id;

            const BufferCreateInfo& createInfo = m_bufferCreateInfoContainer.getDataRef(_hBuf.id);
            Buffer_vk newBuf = vkz::createBuffer(info, m_memProps, m_device, getBufferUsageFlags(createInfo.usage), getMemPropFlags(createInfo.memFlags));
            m_bufferContainer.update_data(_hBuf.id, newBuf);

            ResInteractDesc interact{ createInfo.barrierState };
            m_barrierDispatcher.addBuffer(
                newBuf.buffer
                , { getAccessFlags(interact.access), getPipelineStageFlags(interact.stage) }
                , newBuf.buffer
            );
        }

        const Buffer_vk& newBuf = m_bufferContainer.getIdToData(_hBuf.id);
        uploadBuffer(_hBuf.id, _data, _size);
    }

    void RHIContext_vk::updateCustomFuncData(const PassHandle _hPass, const void* _data, uint32_t _size)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(_data != nullptr);
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

    void RHIContext_vk::createShader(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        ShaderCreateInfo info;
        read(&_reader, info);

        char path[kMaxPathLen];
        read(&_reader, path, info.pathLen);
        path[info.pathLen] = '\0'; // null-terminated string

        Shader_vk shader{};
        bool lsr = loadShader(shader, m_device, path);
        assert(lsr);

        m_shaderContainer.push_back(info.shaderId, shader);
    }

    void RHIContext_vk::createProgram(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        ProgramCreateInfo info;
        read(&_reader, info);

        stl::vector<uint16_t> shaderIds(info.shaderNum);
        read(&_reader, shaderIds.data(), info.shaderNum * sizeof(uint16_t));

        stl::vector<Shader_vk> shaders;
        for (const uint16_t sid : shaderIds)
        {
            shaders.push_back(m_shaderContainer.getIdToData(sid));
        }

        VkPipelineBindPoint bindPoint = getBindPoint(shaders);
        Program_vk prog = vkz::createProgram(m_device, bindPoint, shaders, info.sizePushConstants);

        m_programContainer.push_back(info.progId, prog);
        m_programShaderIds.emplace_back(shaderIds);

        assert(m_programContainer.size() == m_programShaderIds.size());
    }

    void RHIContext_vk::createPass(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        PassMetaData passMeta;
        read(&_reader, passMeta);
        
        size_t vtxBindingSz = passMeta.vertexBindingNum * sizeof(VertexBindingDesc);
        size_t vtxAttributeSz = passMeta.vertexAttributeNum * sizeof(VertexAttributeDesc);

        stl::vector<VertexBindingDesc> passVertexBinding(passMeta.vertexBindingNum);
        read(&_reader, passVertexBinding.data(), (int32_t)vtxBindingSz);
        stl::vector<VertexAttributeDesc> passVertexAttribute(passMeta.vertexAttributeNum);
        read(&_reader, passVertexAttribute.data(), (int32_t)vtxAttributeSz);

        // pipeline spec data
        stl::vector<int> pipelineSpecData(passMeta.pipelineSpecNum);
        read(&_reader, pipelineSpecData.data(), passMeta.pipelineSpecNum * sizeof(int));

        // r/w resources
        stl::vector<uint16_t> writeImageIds(passMeta.writeImageNum);
        read(&_reader, writeImageIds.data(), passMeta.writeImageNum * sizeof(uint16_t));

        stl::vector<uint16_t> readImageIds(passMeta.readImageNum);
        read(&_reader, readImageIds.data(), passMeta.readImageNum * sizeof(uint16_t));

        stl::vector<uint16_t> readBufferIds(passMeta.readBufferNum);
        read(&_reader, readBufferIds.data(), passMeta.readBufferNum * sizeof(uint16_t));
        
        stl::vector<uint16_t> writeBufferIds(passMeta.writeBufferNum);
        read(&_reader, writeBufferIds.data(), passMeta.writeBufferNum * sizeof(uint16_t));

        const size_t interactSz =
            passMeta.writeImageNum +
            passMeta.readImageNum +
            passMeta.readBufferNum +
            passMeta.writeBufferNum;

        stl::vector<ResInteractDesc> interacts(interactSz);
        read(&_reader, interacts.data(), (uint32_t)(interactSz * sizeof(ResInteractDesc)));

        // samplers
        stl::vector<uint16_t> sampleImageIds(passMeta.sampleImageNum);
        stl::vector<uint16_t> samplerIds(passMeta.sampleImageNum);
        read(&_reader, sampleImageIds.data(), passMeta.sampleImageNum * sizeof(uint16_t));
        read(&_reader, samplerIds.data(), passMeta.sampleImageNum * sizeof(uint16_t));

        // write op aliases
        stl::vector<CombinedResID> writeOpAliasInIds(passMeta.writeBufAliasNum + passMeta.writeImgAliasNum);
        read(&_reader, writeOpAliasInIds.data(), (passMeta.writeBufAliasNum + passMeta.writeImgAliasNum) * sizeof(CombinedResID));

        stl::vector<CombinedResID> writeOpAliasOutIds(passMeta.writeBufAliasNum + passMeta.writeImgAliasNum);
        read(&_reader, writeOpAliasOutIds.data(), (passMeta.writeBufAliasNum + passMeta.writeImgAliasNum) * sizeof(CombinedResID));


        VkPipeline pipeline{};
        if (passMeta.queue == PassExeQueue::graphics)
        {
            // create pipeline
            const uint16_t progIdx = (uint16_t)m_programContainer.getIndex(passMeta.programId);
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

            PipelineConfigs_vk configs{ passMeta.pipelineConfig.enableDepthTest, passMeta.pipelineConfig.enableDepthWrite, getCompareOp(passMeta.pipelineConfig.depthCompOp) };

            VkPipelineCache cache{};
            pipeline = vkz::createGraphicsPipeline(m_device, cache, program.layout, renderInfo, shaders, hasVIS ? &vtxInputCreateInfo : nullptr, pipelineSpecData, configs);
            assert(pipeline);
        }
        else if (passMeta.queue == PassExeQueue::compute)
        {
            // create pipeline
            const uint16_t progIdx = (uint16_t)m_programContainer.getIndex(passMeta.programId);
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
            pipeline = vkz::createComputePipeline(m_device, cache, program.layout, shader, pipelineSpecData);
            assert(pipeline);
        }

        // fill pass info
        PassInfo_vk passInfo{};
        passInfo.passId = passMeta.passId;
        passInfo.pipeline = pipeline;

        passInfo.vertexBufferId = passMeta.vertexBufferId;
        passInfo.indexBufferId = passMeta.indexBufferId;
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
            , UniDataContainer< uint16_t, BarrierState_vk>& _container
            , stl::vector<std::pair<uint32_t, CombinedResID>>& _bindings
            , const ResourceType _type
            ) {
                for (uint32_t ii = 0; ii < _ids.size(); ++ii)
                {
                    _container.push_back(_ids[ii], getBarrierState(interacts[offset + ii]));

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
            passInfo.imageToSamplerIds.push_back(sampleImageIds[ii], samplerIds[ii]);
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
                passInfo.writeOpInToOut.push_back(writeOpAliasInIds[ii], writeOpAliasOutIds[ii]);
            }
        }

        m_passContainer.push_back(passInfo.passId, passInfo);
    }

    void RHIContext_vk::createImage(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        ImageCreateInfo info;
        read(&_reader, info);

        ImageAliasInfo* resArr = new ImageAliasInfo[info.resCount];
        read(&_reader, resArr, sizeof(ImageAliasInfo) * info.resCount);

        // so the res handle should map to the real buffer array
        stl::vector<ImageAliasInfo> infoList(resArr, resArr + info.resCount);

        ImgInitProps_vk initPorps = getImageInitProp(info, m_imageFormat, m_depthFormat);

        stl::vector<Image_vk> images;
        vkz::createImage(images, infoList, m_device, m_memProps, initPorps);
        assert(images.size() == info.resCount);

        m_imageInitPropContainer.push_back(info.imgId, info);

        for (int ii = 0; ii < info.resCount; ++ii)
        {
            m_imageContainer.push_back(resArr[ii].imgId, images[ii]);
            m_aliasToBaseImages.push_back(resArr[ii].imgId, info.imgId);
            m_imgIdToAliasGroupIdx.insert({ resArr[ii].imgId, (uint32_t)m_imgAliasGroups.size() });
        }
        m_imgAliasGroups.emplace_back(infoList);


        const Image_vk& baseImage = getImage(info.imgId, true);
        for (uint32_t ii = 0; ii < info.viewCount; ++ii)
        {
            const ImageViewHandle imgView = info.mipViews[ii];
            const ImageViewDesc& viewDesc = m_imageViewDescContainer.getIdToData(imgView.id);

            VkImageView view_vk = vkz::createImageView(m_device, baseImage.image, baseImage.format, viewDesc.baseMip, viewDesc.mipLevels);
            m_imageViewContainer.push_back(imgView.id, view_vk);
        }

        m_imgToViewGroupIdx.insert({ info.imgId, (uint16_t)m_imgViewGroups.size() });

        stl::vector<ImageViewHandle> viewHandles(info.mipViews, info.mipViews + info.viewCount);
        m_imgViewGroups.push_back(viewHandles);

        for (int ii = 0; ii < info.resCount; ++ii)
        {
            ResInteractDesc interact{ info.barrierState };

            m_barrierDispatcher.addImage(images[ii].image
                , getImageAspectFlags(images[ii].aspectMask)
                , { getAccessFlags(interact.access), getImageLayout(interact.layout) ,getPipelineStageFlags(interact.stage) }
                , baseImage.image
            );
        }

        if (info.data != nullptr)
        {
            uploadImage(info.imgId, info.data, info.size);
        }
        VKZ_DELETE_ARRAY(resArr);
    }

    void RHIContext_vk::createBuffer(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        BufferCreateInfo info;
        read(&_reader, info);

        BufferAliasInfo* resArr = new BufferAliasInfo[info.resCount];
        read(&_reader, resArr, sizeof(BufferAliasInfo) * info.resCount);

        // so the res handle should map to the real buffer array
        stl::vector<BufferAliasInfo> infoList(resArr, resArr + info.resCount);

        stl::vector<Buffer_vk> buffers;
        vkz::createBuffer(buffers, infoList, m_memProps, m_device, getBufferUsageFlags(info.usage), getMemPropFlags(info.memFlags));

        assert(buffers.size() == info.resCount);

        for (int ii = 0; ii < info.resCount; ++ii)
        {
            buffers[ii].fillVal = info.fillVal;
            m_bufferContainer.push_back(resArr[ii].bufId, buffers[ii]);
            m_aliasToBaseBuffers.push_back(resArr[ii].bufId, info.bufId);

            m_bufIdToAliasGroupIdx.insert({ resArr[ii].bufId, (uint16_t)m_bufAliasGroups.size() });
        }
        m_bufAliasGroups.emplace_back(infoList);

        m_bufferCreateInfoContainer.push_back(info.bufId, info);

        const Buffer_vk& baseBuffer = getBuffer(info.bufId);
        for (int ii = 0; ii < info.resCount; ++ii)
        {
            ResInteractDesc interact{ info.barrierState };
            m_barrierDispatcher.addBuffer(buffers[ii].buffer
                , { getAccessFlags(interact.access), getPipelineStageFlags(interact.stage) }
                , baseBuffer.buffer
            );
        }

        VKZ_DELETE_ARRAY(resArr);
        
        // initialize buffer
        if (info.data != nullptr)
        {
            uploadBuffer(info.bufId, info.data, info.size);
        }
        else
        {
            fillBuffer(info.bufId, info.fillVal, info.size);
        }
    }

    void RHIContext_vk::createSampler(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        SamplerMetaData meta;
        read(&_reader, meta);

        VkSampler sampler = vkz::createSampler(m_device, getSamplerReductionMode( meta.reductionMode));
        assert(sampler);

        m_samplerContainer.push_back(meta.samplerId, sampler);
    }

    void RHIContext_vk::createImageView(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        ImageViewDesc desc;
        read(&_reader, desc);

        // only store the descriptor, will do the real creation in image creation
        m_imageViewDescContainer.push_back(desc.imgViewId, desc);
    }

    void RHIContext_vk::setBackBuffers(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        uint32_t count;
        read(&_reader, count);

        stl::vector<uint16_t> ids(count);
        read(&_reader, ids.data(), count * sizeof(uint16_t));

        for (uint16_t id : ids)
        {
            m_backBufferIds.insert(id);
        }
    }

    void RHIContext_vk::setBrief(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        RHIBrief brief;
        read(&_reader, brief);

        m_brief = brief;
    }

    void RHIContext_vk::createInstance()
    {
        VKZ_ZoneScopedC(Color::indian_red);

        m_instance = vkz::createInstance();
        assert(m_instance);

        volkLoadInstanceOnly(m_instance);
    }

    void RHIContext_vk::createPhysicalDevice()
    {
        VkPhysicalDevice physicalDevices[16];
        uint32_t deviceCount = sizeof(physicalDevices) / sizeof(physicalDevices[0]);
        VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices));

        m_phyDevice = vkz::pickPhysicalDevice(physicalDevices, deviceCount);
        assert(m_phyDevice);
    }

    void RHIContext_vk::recreateBackBuffers()
    {
        // destroy images
        for (uint16_t id : m_backBufferIds)
        {
            uint32_t aliasGroupIdx = m_imgIdToAliasGroupIdx.find(id)->second;
            const stl::vector<ImageAliasInfo>& aliases = m_imgAliasGroups[aliasGroupIdx];

            stl::vector<Image_vk> images;
            for (ImageAliasInfo aliasInfo : aliases)
            {
                images.push_back(m_imageContainer.getIdToData(aliasInfo.imgId));
            }

            // remove barriers
            for (const Image_vk& img : images)
            {
                m_barrierDispatcher.removeImage(img.image);
            }

            // destroy Image
            vkz::destroyImage(m_device, images);
            
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
        for (uint16_t id : m_backBufferIds)
        {
            uint32_t aliasGroupIdx = m_imgIdToAliasGroupIdx.find(id)->second;
            const stl::vector<ImageAliasInfo>& aliases = m_imgAliasGroups[aliasGroupIdx];

            uint16_t baseId = m_aliasToBaseImages.getIdToData(id);
            ImageCreateInfo& createInfo = m_imageInitPropContainer.getDataRef(baseId);
            createInfo.width = m_swapchain.width;
            createInfo.height = m_swapchain.height;

            ImgInitProps_vk initPorps = getImageInitProp(createInfo, m_imageFormat, m_depthFormat);

            //image
            stl::vector<Image_vk> images;
            vkz::createImage(images, aliases, m_device, m_memProps, initPorps);

            for (uint16_t ii = 0 ; ii < aliases.size(); ++ii)
            {
                m_imageContainer.update_data(aliases[ii].imgId, images[ii]);
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
                    VkImageView view_vk = vkz::createImageView(m_device, baseImg.image, baseImg.format, viewDesc.baseMip, viewDesc.mipLevels);
                    m_imageViewContainer.update_data(view.id, view_vk);
                }
            }

            for (int ii = 0; ii < aliases.size(); ++ii)
            {
                ResInteractDesc interact{ createInfo.barrierState };

                m_barrierDispatcher.addImage(images[ii].image
                    , getImageAspectFlags(images[ii].aspectMask)
                    , { getAccessFlags(interact.access), getImageLayout(interact.layout) ,getPipelineStageFlags(interact.stage) }
                    , baseImg.image
                );
            }
        }
    }

    void RHIContext_vk::uploadBuffer(const uint16_t _bufId, const void* data, uint32_t size)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(size > 0);
        assert(m_scratchBuffer.data);
        assert(m_scratchBuffer.size > size);

        memcpy(m_scratchBuffer.data, data, size);

        const Buffer_vk& buffer = getBuffer(_bufId);

        VK_CHECK(vkResetCommandPool(m_device, m_cmdPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(m_cmdBuffer, &beginInfo));

        VkBufferCopy region = { 0, 0, VkDeviceSize(size) };
        vkCmdCopyBuffer(m_cmdBuffer, m_scratchBuffer.buffer, buffer.buffer, 1, &region);

        m_barrierDispatcher.barrier(buffer.buffer,
            { VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        m_barrierDispatcher.dispatch(m_cmdBuffer);

        VK_CHECK(vkEndCommandBuffer(m_cmdBuffer));

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_cmdBuffer;

        VK_CHECK(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));

        VK_CHECK(vkDeviceWaitIdle(m_device));
    }

    void RHIContext_vk::fillBuffer(const uint16_t _bufId, const uint32_t _value, uint32_t _size)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(_size > 0);

        const Buffer_vk& buf = getBuffer(_bufId);

        VK_CHECK(vkResetCommandPool(m_device, m_cmdPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(m_cmdBuffer, &beginInfo));

        vkCmdFillBuffer(m_cmdBuffer, buf.buffer, 0, _size, _value);

        m_barrierDispatcher.barrier(buf.buffer,
            //{ VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT },
            { VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        m_barrierDispatcher.dispatch(m_cmdBuffer);

        VK_CHECK(vkEndCommandBuffer(m_cmdBuffer));

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_cmdBuffer;
        VK_CHECK(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));

        VK_CHECK(vkDeviceWaitIdle(m_device));
    }

    void RHIContext_vk::uploadImage(const uint16_t _imgId, const void* data, uint32_t size)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(size > 0);
        assert(m_scratchBuffer.data);
        assert(m_scratchBuffer.size > size);

        memcpy(m_scratchBuffer.data, data, size);

        const Image_vk& image = getImage(_imgId);

        VK_CHECK(vkResetCommandPool(m_device, m_cmdPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(m_cmdBuffer, &beginInfo));

        m_barrierDispatcher.barrier(image.image
            , image.aspectMask
            , { VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );

        m_barrierDispatcher.dispatch(m_cmdBuffer);

        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = image.aspectMask;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = image.width;
        region.imageExtent.height = image.height;
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(m_cmdBuffer, m_scratchBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);


        VK_CHECK(vkEndCommandBuffer(m_cmdBuffer));

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_cmdBuffer;

        VK_CHECK(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));

        VK_CHECK(vkDeviceWaitIdle(m_device));
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

        //message(info, "+++++++ mismatched barriers +++++++++++++++++++++");
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

                VkImageView view = img.defalutImgView;
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

    const vkz::Shader_vk& RHIContext_vk::getShader(const ShaderHandle _hShader) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        return m_shaderContainer.getIdToData(_hShader.id);
    }

    const vkz::Program_vk& RHIContext_vk::getProgram(const PassHandle _hPass) const
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
            colorAttachments[ii].imageView = colorTarget.defalutImgView;
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
            depthAttachment.imageView = depthTarget.defalutImgView;
        }

        VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea.extent.width = m_swapchain.width;
        renderingInfo.renderArea.extent.height = m_swapchain.height;
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

        VkImageView view = img.defalutImgView;
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
        VkViewport viewport = { 0.f, float(m_swapchain.height), float(m_swapchain.width), -float(m_swapchain.height), 0.f, 1.f };
        VkRect2D scissor = { {0, 0}, {m_swapchain.width, m_swapchain.height } };

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
        const uint16_t progIdx = (uint16_t)m_programContainer.getIndex(passInfo.programId);
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
            //src,
            { VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );
        
        // swapchain image
        const VkImage& swapImg = m_swapchain.images[_swapImgIdx];

        m_barrierDispatcher.barrier(
            swapImg, VK_IMAGE_ASPECT_COLOR_BIT,
            //{ 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );

        m_barrierDispatcher.dispatch(m_cmdBuffer);

        // copy to swapchain
        VkImageCopy copyRegion = {};
        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.extent = { m_swapchain.width, m_swapchain.height, 1 };

        vkCmdCopyImage(
            m_cmdBuffer,
            presentImg.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            swapImg, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copyRegion
        );

        // add present barrier
        m_barrierDispatcher.barrier(
            swapImg, VK_IMAGE_ASPECT_COLOR_BIT,
            //{ VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT },
            { 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT }
        );

        m_barrierDispatcher.dispatch(m_cmdBuffer);
    }

    void BarrierDispatcher::addImage(const VkImage _img, const ImageAspectFlags _aspect, BarrierState_vk _barrierState, const VkImage _baseImg/* = 0*/)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(m_imgAspectFlags.find(_img) == end(m_imgAspectFlags));
        assert(m_imgBarrierStatus.find(_img) == end(m_imgBarrierStatus));

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

    void BarrierDispatcher::removeBuffer(const VkBuffer _buf)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        m_aliasToBaseBuffers.erase(_buf);
        m_bufBarrierStatus.erase(_buf);
    }

    void BarrierDispatcher::removeImage(const VkImage _img)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        m_aliasToBaseImages.erase(_img);
        m_imgAspectFlags.erase(_img);
    }

    void BarrierDispatcher::addBuffer(const VkBuffer _buf, BarrierState_vk _barrierState, const VkBuffer _baseBuf/* = 0*/)
    {
        VKZ_ZoneScopedC(Color::indian_red);

        assert(m_bufBarrierStatus.find(_buf) == end(m_bufBarrierStatus));

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

        assert(m_bufBarrierStatus.find(_buf) != end(m_bufBarrierStatus));

        const size_t idx = getElemIndex(m_dispatchBuffers, _buf);
        if (kInvalidHandle == idx)
        {
            m_dispatchBuffers.emplace_back(_buf);

            const BarrierState_vk src = m_bufBarrierStatus.at(_buf);
            m_srcBufBarriers.emplace_back(src);

            m_dstBufBarriers.push_back({ _dst.accessMask , _dst.stageMask });
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

        assert(m_imgAspectFlags.find(_img) != end(m_imgAspectFlags));
        assert(m_imgBarrierStatus.find(_img) != end(m_imgBarrierStatus));

        const size_t idx = getElemIndex(m_dispatchImages, _img);

        if (kInvalidHandle == idx)
        {
            m_dispatchImages.emplace_back(_img);

            m_imgAspects.emplace_back(_aspect);

            BarrierState_vk src = m_imgBarrierStatus.at(_img);
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

            m_imgAspectFlags.at(img) = aspect;
            m_imgBarrierStatus.at(img) = dst;

            if (m_aliasToBaseImages.find(img) != end(m_aliasToBaseImages))
            {
                const VkImage baseImg = m_aliasToBaseImages.at(img);
                m_baseImgAspectFlags.at(baseImg) = aspect;
                m_baseImgBarrierStatus.at(baseImg) = dst;
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

            m_bufBarrierStatus.at(buf) = dst;

            if(m_aliasToBaseBuffers.find(buf) != end(m_aliasToBaseBuffers))
            {
                const VkBuffer baseBuf = m_aliasToBaseBuffers.at(buf);
                m_baseBufBarrierStatus.at(baseBuf) = dst;
            }
        }
        
        pipelineBarrier(_cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, (uint32_t)bufBarriers.size(), bufBarriers.data(), (uint32_t)imgBarriers.size(), imgBarriers.data());
        
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

        return m_imgBarrierStatus.at(_img).imgLayout;
        
    }

    const BarrierState_vk BarrierDispatcher::getBarrierState(const VkImage _img) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        return m_imgBarrierStatus.at(_img);
    }

    const BarrierState_vk BarrierDispatcher::getBarrierState(const VkBuffer _buf) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        return m_bufBarrierStatus.at(_buf);
    }

    const BarrierState_vk BarrierDispatcher::getBaseBarrierState(const VkImage _img) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        return m_baseImgBarrierStatus.at(_img);
    }

    const BarrierState_vk BarrierDispatcher::getBaseBarrierState(const VkBuffer _buf) const
    {
        VKZ_ZoneScopedC(Color::indian_red);

        return m_baseBufBarrierStatus.at(_buf);
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

} // namespace vkz