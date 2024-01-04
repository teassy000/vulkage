#include "common.h"
#include "macro.h"

#include "memory_operation.h"
#include "config.h"
#include "vkz.h"
#include "util.h"

#include "rhi_context_vk.h"

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>


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

    VkImageAspectFlags getImageAspectFlags(ImageAspectFlags _aspectFlags)
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

    VkPipelineBindPoint getBindPoint(const std::vector<Shader_vk>& shaders)
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

    bool getVertexInputState(VkPipelineVertexInputStateCreateInfo& _out, const std::vector<VertexBindingDesc>& _bindings, const std::vector<VertexAttributeDesc>& _attributes)
    {
        if (_bindings.empty() && _attributes.empty())
        {
            return false;
        }

        std::vector<VkVertexInputBindingDescription> bindings;
        for (const auto& bind : _bindings)
        {
            bindings.push_back({ bind.binding, bind.stride, getInputRate(bind.inputRate) });
        }

        std::vector<VkVertexInputAttributeDescription> attributes;
        for (const auto& attr : _attributes)
        {
            attributes.push_back({ attr.location, attr.binding, getFormat(attr.format), attr.offset });
        }

        _out.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        _out.vertexBindingDescriptionCount = (uint32_t)bindings.size();
        _out.pVertexBindingDescriptions = bindings.data();
        _out.vertexAttributeDescriptionCount = (uint32_t)attributes.size();
        _out.pVertexAttributeDescriptions = attributes.data();

        return true;
    }

    void enumrateDeviceExtPorps(VkPhysicalDevice physicalDevice, std::vector<VkExtensionProperties>& availableExtensions)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        availableExtensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
    }

    bool checkExtSupportness(const std::vector<VkExtensionProperties>& props, const char* extName, bool print = true)
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

    RHIContext_vk::RHIContext_vk(AllocatorI* _allocator, RHI_Config _config)
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
#if _DEBUG
        , m_debugCallback{ VK_NULL_HANDLE }
#endif
    {
        init(_config);
    }

    RHIContext_vk::~RHIContext_vk()
    {

    }

    void RHIContext_vk::init(RHI_Config _config)
{
        int rc = glfwInit();
        assert(rc);

        VK_CHECK(volkInitialize());

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        this->createInstance();

#if _DEBUG
        m_debugCallback = registerDebugCallback(m_instance);
#endif
        createPhysicalDevice();

        std::vector<VkExtensionProperties> supportedExtensions;
        enumrateDeviceExtPorps(m_phyDevice, supportedExtensions);

        m_supportMeshShading = checkExtSupportness(supportedExtensions, VK_EXT_MESH_SHADER_EXTENSION_NAME, false);

        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(m_phyDevice, &props);
        assert(props.limits.timestampPeriod);

        m_gfxFamilyIdx = getGraphicsFamilyIndex(m_phyDevice);
        assert(m_gfxFamilyIdx != VK_QUEUE_FAMILY_IGNORED);

        m_device = vkz::createDevice(m_instance, m_phyDevice, m_gfxFamilyIdx, m_supportMeshShading);
        assert(m_device);
        
        // only single device used in this application.
        volkLoadDevice(m_device);

        // TODO: move this to another file
        m_pWindow = glfwCreateWindow(_config.windowWidth, _config.windowHeight, "mesh_shading_demo", 0, 0);
        assert(m_pWindow);

        /*
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mousekeyCallback);
        glfwSetCursorPosCallback(window, mouseMoveCallback);
        */

        m_surface = createSurface(m_instance, m_pWindow);
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
        
    }

    void RHIContext_vk::render()
    {
        if (m_passContainer.size() < 1)
        {
            message(DebugMessageType::error, "no pass needs to execute in context!");
            return;
        }

        while (!glfwWindowShouldClose(m_pWindow))
        {
            glfwPollEvents();

            int newWindowWidth = 0, newWindowHeight = 0;
            glfwGetWindowSize(m_pWindow, &newWindowWidth, &newWindowHeight);

            SwapchainStatus_vk swapchainStatus = resizeSwapchainIfNecessary(m_swapchain, m_phyDevice, m_device, m_surface, m_gfxFamilyIdx, m_imageFormat);
            if (swapchainStatus == SwapchainStatus_vk::not_ready) {
                continue;
            }

            if(swapchainStatus == SwapchainStatus_vk::resize)
            {
                // TODO: recreate images
            }

            uint32_t imageIndex = 0;
            VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain.swapchain, ~0ull, m_acquirSemaphore, VK_NULL_HANDLE, &imageIndex));

            VK_CHECK(vkResetCommandPool(m_device, m_cmdPool, 0));

            VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            VK_CHECK(vkBeginCommandBuffer(m_cmdBuffer, &beginInfo));

            // render passes
            for (size_t ii = 0; ii < m_passContainer.size(); ++ii)
            {
                uint16_t passId = m_passContainer.getIdAt(ii);
                createBarriers(passId);
                exeutePass(passId);
                createBarriers(passId, true); // flush
            }

            // copy to swapchain
            copyToSwapchain(imageIndex);

            VK_CHECK(vkEndCommandBuffer(m_cmdBuffer));
            
            
            // submit
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

            VK_CHECK(vkDeviceWaitIdle(m_device)); // TODO: a fence here?
        }
    }

    void RHIContext_vk::createShader(MemoryReader& _reader)
    {
        ShaderCreateInfo info;
        read(&_reader, info);

        char path[kMaxPathLen];
        read(&_reader, path, info.pathLen);
        path[info.pathLen] = '\0'; // null-terminated string

        Shader_vk shader{};
        bool lsr = loadShader(shader, m_device, path);
        assert(lsr);

        m_shaderContainer.addData(info.shaderId, shader);
    }

    void RHIContext_vk::createProgram(MemoryReader& _reader)
    {
        ProgramCreateInfo info;
        read(&_reader, info);

        void* mem = alloc(getAllocator(), info.shaderNum * sizeof(uint16_t));
        read(&_reader, mem, info.shaderNum * sizeof(uint16_t));

        std::vector<uint16_t> shaderIds((uint16_t*)mem, (uint16_t*)mem + info.shaderNum);

        std::vector<Shader_vk> shaders;
        for (const uint16_t sid : shaderIds)
        {
            shaders.push_back(m_shaderContainer.getIdToData(sid));
        }

        VkPipelineBindPoint bindPoint = getBindPoint(shaders);
        Program_vk prog = vkz::createProgram(m_device, bindPoint, shaders, info.sizePushConstants);

        m_programContainer.addData(info.progId, prog);
        m_programShaderIds.emplace_back(shaderIds);

        assert(m_programContainer.size() == m_programShaderIds.size());
    }

    void RHIContext_vk::createPass(MemoryReader& _reader)
    {
        PassCreateInfo createInfo;
        read(&_reader, createInfo);
        
        size_t bindingSz = createInfo.vertexBindingNum * sizeof(VertexBindingDesc);
        size_t attributeSz = createInfo.vertexAttributeNum * sizeof(VertexAttributeDesc);
        size_t totolSz = bindingSz + attributeSz;
        
        // vertex binding and attribute
        void* mem = alloc(getAllocator(), totolSz);
        read(&_reader, mem, (int32_t)totolSz);

        std::vector<VertexBindingDesc> passVertexBinding{ (VertexBindingDesc*)mem, ((VertexBindingDesc*)mem) + createInfo.vertexBindingNum };
        std::vector<VertexAttributeDesc> passVertexAttribute{ (VertexAttributeDesc*)((char*)mem + bindingSz), ((VertexAttributeDesc*)mem) + createInfo.vertexBindingNum };

        // constants
        std::vector<int> pushConstants(createInfo.pushConstantNum);
        read(&_reader, pushConstants.data(), createInfo.pushConstantNum * sizeof(int));

        // r/w resurces
        std::vector<uint16_t> writeColorIds(createInfo.writeColorNum);
        read(&_reader, writeColorIds.data(), createInfo.writeColorNum * sizeof(uint16_t));

        std::vector<uint16_t> readImageIds(createInfo.readImageNum);
        read(&_reader, readImageIds.data(), createInfo.readImageNum * sizeof(uint16_t));

        std::vector<uint16_t> readBufferIds(createInfo.readBufferNum);
        read(&_reader, readBufferIds.data(), createInfo.readBufferNum * sizeof(uint16_t));
        
        std::vector<uint16_t> writeBufferIds(createInfo.writeBufferNum);
        read(&_reader, writeBufferIds.data(), createInfo.writeBufferNum * sizeof(uint16_t));

        const size_t interactSz =
            1 + // write depth
            createInfo.writeColorNum +
            createInfo.readImageNum +
            createInfo.readBufferNum +
            createInfo.writeBufferNum;

        std::vector<ResInteractDesc> interacts(interactSz);
        read(&_reader, interacts.data(), (uint32_t)(interactSz * sizeof(ResInteractDesc)));

        // create pipeline
        const uint16_t progIdx = (uint16_t)m_programContainer.getIndex(createInfo.programId);
        const Program_vk& program = m_programContainer.getIdToData(createInfo.programId);
        const std::vector<uint16_t>& shaderIds = m_programShaderIds[progIdx];

        VkPipeline pipeline{};
        if (createInfo.queue == PassExeQueue::graphics)
        {
            VkPipelineRenderingCreateInfo renderInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
            renderInfo.colorAttachmentCount = createInfo.writeColorNum;
            renderInfo.pColorAttachmentFormats = &m_imageFormat;
            renderInfo.depthAttachmentFormat = m_depthFormat;

            std::vector< Shader_vk> shaders;
            for (const uint16_t sid : shaderIds)
            {
                shaders.push_back(m_shaderContainer.getIdToData(sid));
            }

            VkPipelineVertexInputStateCreateInfo vtxInputCreateInfo{};
            bool hasVIS = getVertexInputState(vtxInputCreateInfo, passVertexBinding, passVertexAttribute);


            PipelineConfigs_vk configs{ createInfo.pipelineConfig.enableDepthTest, createInfo.pipelineConfig.enableDepthWrite, getCompareOp(createInfo.pipelineConfig.depthCompOp) };

            VkPipelineCache cache{};
            pipeline = vkz::createGraphicsPipeline(m_device, cache, program.layout, renderInfo, shaders, hasVIS ? &vtxInputCreateInfo : nullptr, pushConstants, configs);
        }
        else if (createInfo.queue == PassExeQueue::compute)
        {
            std::vector< Shader_vk> shaders;
            for (const uint16_t sid : shaderIds)
            {
                shaders.push_back(m_shaderContainer.getIdToData(sid));
            }

            assert(shaderIds.size() == 1);
            Shader_vk shader = m_shaderContainer.getIdToData(shaderIds[0]);

            VkPipelineCache cache{};
            pipeline = vkz::createComputePipeline(m_device, cache, program.layout, shader, pushConstants);
        }

        assert(pipeline);
        
        // fill pass info
        PassInfo_vk passInfo{};
        passInfo.passId = createInfo.passId;
        passInfo.pipeline = pipeline;

        passInfo.vertexBufferId = createInfo.vertexBufferId;
        passInfo.indexBufferId = createInfo.indexBufferId;
        passInfo.writeDepthId = createInfo.writeDepthId;

        // desc part
        passInfo.programId = createInfo.programId;
        passInfo.queue = createInfo.queue;
        passInfo.vertexBindingNum = createInfo.vertexBindingNum;
        passInfo.vertexAttributeNum = createInfo.vertexAttributeNum;
        passInfo.vertexBindingInfos = createInfo.vertexBindingInfos;
        passInfo.vertexAttributeInfos = createInfo.vertexAttributeInfos;
        passInfo.pushConstantNum = createInfo.pushConstantNum;
        passInfo.pushConstants = createInfo.pushConstants;
        passInfo.pipelineConfig = createInfo.pipelineConfig;

        // r/w res part
        auto getBarrierState = [&](const ResInteractDesc& _desc) -> BarrierState_vk {
                BarrierState_vk state{};
                state.accessMask = getAccessFlags(_desc.access);
                state.imgLayout = getImageLayout(_desc.layout);
                state.stageMask = getPipelineStageFlags(_desc.stage);

                return state;
            };

        size_t offset = 1; // the depth is the first one
        auto addToPass = [&interacts, &offset, getBarrierState](const std::vector<uint16_t>& _ids, UniDataContainer< uint16_t, BarrierState_vk>& _container) {
                for (uint32_t ii = 0; ii < _ids.size(); ++ii)
                {
                    _container.addData(_ids[ii], getBarrierState(interacts[offset + ii]));
                    offset++;
                }
            };


        passInfo.writeDepth = std::make_pair(passInfo.writeDepthId, getBarrierState(interacts[0]));

        addToPass(writeColorIds, passInfo.writeColors);
        addToPass(readImageIds, passInfo.readImages);
        addToPass(readBufferIds, passInfo.readBuffers);
        addToPass(writeBufferIds, passInfo.writeBuffers);

        assert(offset == interacts.size());

        m_passContainer.addData(passInfo.passId, passInfo);
    }

    void RHIContext_vk::createImage(MemoryReader& _reader)
    {
        ImageCreateInfo info;
        read(&_reader, info);

        ImageAliasInfo* resArr = new ImageAliasInfo[info.aliasNum];
        read(&_reader, resArr, sizeof(ImageAliasInfo) * info.aliasNum);

        // so the res handle should map to the real buffer array
        std::vector<ImageAliasInfo> infoList(resArr, resArr + info.aliasNum);

        ImgInitProps_vk initPorps{};
        initPorps.level = info.mips;
        initPorps.width = info.width;
        initPorps.height = info.height;
        initPorps.depth = info.depth;
        initPorps.format = getValidFormat(info.format, info.usage, m_imageFormat, m_depthFormat);
        initPorps.usage = getImageUsageFlags(info.usage);
        initPorps.type = getImageType(info.type);
        initPorps.layout = getImageLayout(info.layout);
        initPorps.viewType = getImageViewType( info.viewType);
        initPorps.aspectMask = getImageAspectFlags(info.aspectFlags);

        std::vector<Image_vk> images;
        vkz::createImage(images, infoList, m_device, m_memProps, initPorps);

        assert(images.size() == info.aliasNum);

        for (int i = 0; i < info.aliasNum; ++i)
        {
            m_imageContainer.addData(resArr[i].imgId, images[i]);

            ResInteractDesc interact{ info.barrierState };
            //m_imgBarrierStatus.addData(resArr[i].imgId, interact);

            m_imgBarrierStates.addData(resArr[i].imgId,
                { getAccessFlags(interact.access), getImageLayout(interact.layout), getPipelineStageFlags(interact.stage) }
            );
        }
        
        VKZ_DELETE_ARRAY(resArr);
    }

    void RHIContext_vk::createBuffer(MemoryReader& _reader)
    {
        BufferCreateInfo info;
        read(&_reader, info);

        BufferAliasInfo* resArr = new BufferAliasInfo[info.aliasNum];
        read(&_reader, resArr, sizeof(BufferAliasInfo) * info.aliasNum);

        // so the res handle should map to the real buffer array
        std::vector<BufferAliasInfo> infoList(resArr, resArr + info.aliasNum);

        std::vector<Buffer_vk> buffers;
        vkz::createBuffer(buffers, infoList, m_memProps, m_device, getBufferUsageFlags(info.usage), getMemPropFlags(info.memFlags));

        assert(buffers.size() == info.aliasNum);

        for (int i = 0; i < info.aliasNum; ++i)
        {
            m_bufferContainer.addData(resArr[i].bufId, buffers[i]);

            ResInteractDesc interact{ info.barrierState };
            //m_bufBarrierStatus.addData(resArr[i].bufId, interact);

            m_bufBarrierStates.addData(resArr[i].bufId, 
                { getAccessFlags(interact.access), getPipelineStageFlags(interact.stage) }
            );
        }

        VKZ_DELETE_ARRAY(resArr);
        
        // try to upload buffer
        if (info.data != nullptr)
        {
            vkz::uploadBuffer(m_device, m_cmdPool, m_cmdBuffer, m_queue, buffers[0], m_scratchBuffer, info.data, info.size);
        }
    }

    void RHIContext_vk::setBrief(MemoryReader& _reader)
    {
        RHIBrief brief;
        read(&_reader, brief);

        m_brief = brief;
    }

    void RHIContext_vk::createInstance()
    {
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

    void RHIContext_vk::createBarriers(uint16_t _passId, bool _flush /*= false*/)
    {
        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        // image barriers
        std::vector<uint16_t>        imgIds;
        std::vector<BarrierState_vk> srcImgInteract;
        std::vector<BarrierState_vk> dstImgInteract;

        // buffer barriers
        std::vector<uint16_t>        bufIds;
        std::vector<BarrierState_vk> srcBufInteract;
        std::vector<BarrierState_vk> dstBufInteract;

        auto addBarrier = [&](const uint16_t id, const BarrierState_vk& src, const BarrierState_vk& dst
            , std::vector<uint16_t>& ids, std::vector<BarrierState_vk>& srcInteract, std::vector<BarrierState_vk>& dstInteract)
            {
                if (src != dst)
                {
                    ids.emplace_back(id);
                    srcInteract.emplace_back(src);
                    dstInteract.emplace_back(dst);
                }
            };

        // depth
        if (kInvalidHandle != passInfo.writeDepth.first)
        {
            uint16_t depth = passInfo.writeDepth.first;
            assert(m_imgBarrierStates.exist(depth));
            BarrierState_vk src = m_imgBarrierStates.getIdToData(depth);
            BarrierState_vk dst = passInfo.writeDepth.second;

            addBarrier(depth, src, dst, imgIds, srcImgInteract, dstImgInteract);
        }

        // write colors
        for (uint32_t ii = 0; ii < passInfo.writeColors.size(); ++ii)
        {
            uint16_t id = passInfo.writeColors.getIdAt(ii);
            assert(m_imgBarrierStates.exist(id));
            BarrierState_vk src = m_imgBarrierStates.getIdToData(id);
            BarrierState_vk dst = passInfo.writeColors.getIdToData(id);

            addBarrier(id, src, dst, imgIds, srcImgInteract, dstImgInteract);
        }

        // write buffers
        for (uint32_t ii = 0; ii < passInfo.writeBuffers.size(); ++ii)
        {
            uint16_t id = passInfo.writeBuffers.getIdAt(ii);
            assert(m_bufBarrierStates.exist(id));
            BarrierState_vk src = m_bufBarrierStates.getIdToData(id);
            BarrierState_vk dst = passInfo.writeBuffers.getIdToData(id);

            addBarrier(id, src, dst, bufIds, srcBufInteract, dstBufInteract);
        }

        if (!_flush)
        {
            // read images
            for (uint32_t ii = 0; ii < passInfo.readImages.size(); ++ii)
            {
                uint16_t id = passInfo.readImages.getIdAt(ii);
                assert(m_imgBarrierStates.exist(id));
                BarrierState_vk src = m_imgBarrierStates.getIdToData(id);
                BarrierState_vk dst = passInfo.readImages.getIdToData(id);

                addBarrier(id, src, dst, imgIds, srcImgInteract, dstImgInteract);
            }

            // read buffers
            for (uint32_t ii = 0; ii < passInfo.readBuffers.size(); ++ii)
            {
                uint16_t id = passInfo.readBuffers.getIdAt(ii);
                assert(m_bufBarrierStates.exist(id));
                BarrierState_vk src = m_bufBarrierStates.getIdToData(id);
                BarrierState_vk dst = passInfo.readBuffers.getIdToData(id);

                addBarrier(id, src, dst, bufIds, srcBufInteract, dstBufInteract);
            }
        }

        assert(srcImgInteract.size() == dstImgInteract.size());
        assert(srcBufInteract.size() == dstBufInteract.size());

        // add image barriers
        for (uint32_t ii = 0; ii < srcImgInteract.size(); ++ii)
        {
            uint16_t imgId = imgIds[ii];

            const Image_vk& img = m_imageContainer.getIdToData(imgId);

            BarrierState_vk vkDst = m_barrierDispatcher.addImgBarrier(
                img.image, img.aspectMask,
                srcImgInteract[ii],
                dstImgInteract[ii]
            );

            m_imgBarrierStates.updateData(imgId, vkDst);
        }

        // add buffer barriers
        for (uint32_t ii = 0; ii < srcBufInteract.size(); ++ii)
        {
            uint16_t bufId = bufIds[ii];

            const Buffer_vk& buf = m_bufferContainer.getIdToData(bufId);

            BarrierState_vk vkDst = m_barrierDispatcher.addBufBarrier(
                buf.buffer,
                srcBufInteract[ii],
                dstBufInteract[ii]
            );

            m_bufBarrierStates.updateData(bufId, vkDst);

        }
    }

    void RHIContext_vk::exeutePass(const uint16_t _passId)
    {
        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        m_barrierDispatcher.dispatch(m_cmdBuffer);

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
        else
        {
            message(DebugMessageType::error, "not a valid execute queue! where does this pass belong?");
        }

    }

    void RHIContext_vk::exeGraphic(const uint16_t _passId)
    {
        const PassInfo_vk& passInfo = m_passContainer.getIdToData(_passId);

        VkClearColorValue color = { 33.f / 255.f, 200.f / 255.f, 242.f / 255.f, 1 };
        VkClearDepthStencilValue depth = { 0.f, 0 };

        std::vector<VkRenderingAttachmentInfo> colorAttachments(passInfo.writeColors.size());
        for (int ii = 0; ii < passInfo.writeColors.size(); ++ii)
        {
            const Image_vk& colorTarget = m_imageContainer.getIdToData(passInfo.writeColors.getIdAt(ii));

            colorAttachments[ii].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            colorAttachments[ii].clearValue.color = color;
            colorAttachments[ii].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachments[ii].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachments[ii].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            colorAttachments[ii].imageView = colorTarget.imageView;
        }

        bool hasDepth = passInfo.writeDepthId != kInvalidHandle;
        VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        if (hasDepth)
        {
            const Image_vk& depthTarget = m_imageContainer.getIdToData(passInfo.writeDepthId);

            depthAttachment.clearValue.depthStencil = depth;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            depthAttachment.imageView = depthTarget.imageView;
        }

        VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea.extent.width = m_swapchain.width;
        renderingInfo.renderArea.extent.height = m_swapchain.height;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = (uint32_t)colorAttachments.size();
        renderingInfo.pColorAttachments = colorAttachments.data();
        renderingInfo.pDepthAttachment = hasDepth ? &depthAttachment : nullptr;

        vkCmdBeginRendering(m_cmdBuffer, &renderingInfo);

        // drawcalls
        VkViewport viewport = { 0.f, float(m_swapchain.height), float(m_swapchain.width), -float(m_swapchain.height), 0.f, 1.f };
        VkRect2D scissor = { {0, 0}, {m_swapchain.width, m_swapchain.height } };

        vkCmdSetViewport(m_cmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(m_cmdBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passInfo.pipeline);


        // TODO: push descriptors by order of binding
        std::vector<DescriptorInfo> descInfos;

        const Program_vk& prog =  m_programContainer.getIdToData(passInfo.programId);
        //vkCmdPushDescriptorSetWithTemplateKHR(m_cmdBuffer, prog.updateTemplate, prog.layout, 0, descInfos.data());
        

        // push constants

        // set vertex buffer
        if (kInvalidHandle != passInfo.vertexBufferId)
        {
            VkDeviceSize offsets[1] = { 0 };
            const Buffer_vk& vertexBuffer = m_bufferContainer.getIdToData(passInfo.vertexBufferId);
            vkCmdBindVertexBuffers(m_cmdBuffer, 0, 1, &vertexBuffer.buffer, offsets);
        }

        // set index buffer
        if (kInvalidHandle != passInfo.indexBufferId)
        {
            const Buffer_vk& indexBuffer = m_bufferContainer.getIdToData(passInfo.indexBufferId);
            vkCmdBindIndexBuffer(m_cmdBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32); // TODO: do I need to expose the index type out?
        }

        vkCmdDraw(m_cmdBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(m_cmdBuffer);
    }

    void RHIContext_vk::exeCompute(const uint16_t _passId)
    {
        assert(0);
    }

    void RHIContext_vk::exeCopy(const uint16_t _passId)
    {
        assert(0);
    }

    void RHIContext_vk::copyToSwapchain(uint32_t _swapImgIdx)
    {
        if (!m_imageContainer.exist(m_brief.presentImageId))
        {
            message(DebugMessageType::error, "Does the presentImageId set correctly?");
            return;
        }
        // add swapchain barrier
        // img to present
        const Image_vk& presentImg = m_imageContainer.getIdToData(m_brief.presentImageId);

        const BarrierState_vk& src2 = m_imgBarrierStates.getIdToData(m_brief.presentImageId);

        BarrierState_vk next2 = m_barrierDispatcher.addImgBarrier(
            presentImg.image, presentImg.aspectMask,
            src2,
            { VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT }
        );

        m_imgBarrierStates.updateData(m_brief.presentImageId, next2);
        
        // swapchain image
        const VkImage& swapImg = m_swapchain.images[_swapImgIdx];

        m_barrierDispatcher.addImgBarrier(
            swapImg, VK_IMAGE_ASPECT_COLOR_BIT,
            { 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
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
        m_barrierDispatcher.addImgBarrier(
            swapImg, VK_IMAGE_ASPECT_COLOR_BIT,
            { VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT },
            { 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT }
        );

        m_barrierDispatcher.dispatch(m_cmdBuffer);
    }

    BarrierState_vk BarrierDispatcher::addBufBarrier(const VkBuffer _buf, const BarrierState_vk& _src, const BarrierState_vk& _dst)
    {
        BarrierState_vk result{};

        uint16_t idx = getElemIndex(m_bufs, _buf);
        if (kInvalidHandle == idx)
        {
            m_bufs.emplace_back(_buf);

            BarrierState_vk src{};
            src.accessMask = _src.accessMask;
            src.stageMask = _src.stageMask;
            m_srcBufBarriers.emplace_back(src);

            BarrierState_vk dst{};
            dst.accessMask = _dst.accessMask;
            dst.stageMask = _dst.stageMask;
            m_dstBufBarriers.emplace_back(dst);

            result = dst;
        }
        else
        {
            // update the exist one
            BarrierState_vk& src = m_srcBufBarriers[idx];
            src.accessMask |= _src.accessMask;
            src.stageMask |= _src.stageMask;

            BarrierState_vk& dst = m_dstBufBarriers[idx];
            dst.accessMask |= _dst.accessMask;
            dst.stageMask |= _dst.stageMask;

            result = dst;
        }

        return result;
    }

    BarrierState_vk BarrierDispatcher::addImgBarrier(const VkImage _img, VkImageAspectFlags _aspect, const BarrierState_vk& _src, const BarrierState_vk& _dst)
    {
        uint16_t idx = getElemIndex(m_imgs, _img);
        BarrierState_vk result{};

        if (kInvalidHandle == idx)
        {
            m_imgs.emplace_back(_img);

            m_imgAspects.emplace_back(_aspect);

            BarrierState_vk src{};
            src.accessMask = _src.accessMask;
            src.stageMask = _src.stageMask;
            src.imgLayout = _src.imgLayout;
            m_srcImgBarriers.emplace_back(src);

            BarrierState_vk dst{};
            dst.accessMask = _dst.accessMask;
            dst.stageMask = _dst.stageMask;
            dst.imgLayout = _dst.imgLayout;
            m_dstImgBarriers.emplace_back(dst);

            result = dst;
        }
        else
        {
            assert(_aspect == m_imgAspects[idx]);
            assert(_src.imgLayout == m_srcImgBarriers[idx].imgLayout); // there must be something wrong if a image has different src layout between two passes

            // update the exist one
            BarrierState_vk& src = m_srcImgBarriers[idx];
            src.accessMask |= _src.accessMask;
            src.stageMask |= _src.stageMask;

            BarrierState_vk& dst = m_dstImgBarriers[idx];
            dst.accessMask |= _dst.accessMask;
            dst.stageMask |= _dst.stageMask;
            dst.imgLayout = _dst.imgLayout; // use the new destinate layout instead

            result = dst;
        }

        return result;
    }

    void BarrierDispatcher::dispatch(const VkCommandBuffer& _cmdBuffer)
    {
        std::vector<VkImageMemoryBarrier2> imgBarriers;
        for (uint32_t ii = 0; ii < m_imgs.size(); ++ii)
        {
            const VkImage& img = m_imgs[ii];
            const VkImageAspectFlags aspect = m_imgAspects[ii];
            BarrierState_vk src = m_srcImgBarriers[ii];
            BarrierState_vk dst = m_dstImgBarriers[ii];

            VkImageMemoryBarrier2 barrier = imageBarrier(
                img, aspect,
                src.accessMask, src.imgLayout, src.stageMask,
                dst.accessMask, dst.imgLayout, dst.stageMask);

            imgBarriers.emplace_back(barrier);
        }

        std::vector<VkBufferMemoryBarrier2> bufBarriers;
        for (uint32_t ii = 0; ii < m_bufs.size(); ++ii)
        {
            const VkBuffer& buf = m_bufs[ii];
            BarrierState_vk src = m_srcImgBarriers[ii];
            BarrierState_vk dst = m_dstImgBarriers[ii];

            VkBufferMemoryBarrier2 barrier = bufferBarrier(
                buf,
                src.accessMask, src.stageMask,
                dst.accessMask, dst.stageMask);

            bufBarriers.emplace_back(barrier);
        }
        
        pipelineBarrier(_cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, (uint32_t)bufBarriers.size(), bufBarriers.data(), (uint32_t)imgBarriers.size(), imgBarriers.data());


        // clear all once barriers dispatched
        clear();
    }

    void BarrierDispatcher::clear()
    {
        m_imgs.clear();
        m_imgAspects.clear();
        m_srcImgBarriers.clear();
        m_dstImgBarriers.clear();
        
        m_bufs.clear();
        m_srcBufBarriers.clear();
        m_dstBufBarriers.clear();
    }

} // namespace vkz