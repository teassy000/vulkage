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

    RHIContext_vk::RHIContext_vk(AllocatorI* _allocator)
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
        init();
    }


    RHIContext_vk::~RHIContext_vk()
    {

    }

    void RHIContext_vk::init()
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
        m_pWindow = glfwCreateWindow(2560, 1440, "mesh_shading_demo", 0, 0);
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

        vkGetPhysicalDeviceMemoryProperties(m_phyDevice, &m_memProps);
    }

    void RHIContext_vk::render()
    {
        while (!glfwWindowShouldClose(m_pWindow))
        {
            glfwPollEvents();

            int newWindowWidth = 0, newWindowHeight = 0;
            glfwGetWindowSize(m_pWindow, &newWindowWidth, &newWindowHeight);

            SwapchainStatus_vk swapchainStatus = resizeSwapchainIfNecessary(m_swapchain, m_phyDevice, m_device, m_surface, m_gfxFamilyIdx, m_imageFormat);
            if (swapchainStatus == SwapchainStatus_vk::not_ready) {
                continue;
            }

            uint32_t imageIndex = 0;
            VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain.swapchain, ~0ull, m_acquirSemaphore, VK_NULL_HANDLE, &imageIndex));

            VK_CHECK(vkResetCommandPool(m_device, m_cmdPool, 0));

            VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            VK_CHECK(vkBeginCommandBuffer(m_cmdBuffer, &beginInfo));

            for (size_t ii = 0; ii < m_passContainer.size(); ++ii)
            {
                passRender(m_passContainer.getId(ii));
            }

            VK_CHECK(vkEndCommandBuffer(m_cmdBuffer));


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

            double waitTimeBegin = glfwGetTime() * 1000.0;
            VK_CHECK(vkDeviceWaitIdle(m_device)); // TODO: a fence here?
            double waitTimeEnd = glfwGetTime() * 1000.0;
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
            shaders.push_back(m_shaderContainer.getData(sid));
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
        size_t offset = bindingSz;
        size_t totolSz = bindingSz + attributeSz;
        
        // vertex binding and attribute
        void* mem = alloc(getAllocator(), totolSz);
        read(&_reader, mem, (int32_t)totolSz);

        std::vector<VertexBindingDesc> passVertexBinding{ (VertexBindingDesc*)mem, ((VertexBindingDesc*)mem) + createInfo.vertexBindingNum };
        std::vector<VertexAttributeDesc> passVertexAttribute{ (VertexAttributeDesc*)((char*)mem + offset), ((VertexAttributeDesc*)mem) + createInfo.vertexBindingNum };

        // constants
        std::vector<int> pushConstants(createInfo.pushConstantNum);
        read(&_reader, pushConstants.data(), createInfo.pushConstantNum * sizeof(int));

        // r/w resurces
        std::vector<uint16_t> writeColorIds(createInfo.writeColorNum);
        read(&_reader, writeColorIds.data(), createInfo.writeColorNum * sizeof(uint16_t));
        std::vector<uint16_t> readImageIds(createInfo.readImageNum);
        read(&_reader, readImageIds.data(), createInfo.readImageNum * sizeof(uint16_t));
        std::vector<uint16_t> rwBufferIds(createInfo.rwBufferNum);
        read(&_reader, rwBufferIds.data(), createInfo.rwBufferNum * sizeof(uint16_t));


        // create pipeline
        const uint16_t progIdx = (uint16_t)m_programContainer.getIndex(createInfo.programId);
        const Program_vk& program = m_programContainer.getData(createInfo.programId);
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
                shaders.push_back(m_shaderContainer.getData(sid));
            }

            VkPipelineVertexInputStateCreateInfo vtxInputCreateInfo{};
            bool hasVIS = getVertexInputState(vtxInputCreateInfo, passVertexBinding, passVertexAttribute);


            PipelineConfigs_vk configs{ createInfo.pipelineConfig.enableDepthTest, createInfo.pipelineConfig.enableDepthWrite, getCompareOp(createInfo.pipelineConfig.depthCompOp) };

            VkPipelineCache cache{};
            pipeline = vkz::createGraphicsPipeline(m_device, cache, program.layout, renderInfo, shaders, hasVIS ? &vtxInputCreateInfo : nullptr, pushConstants, configs);
        }
        else if (createInfo.queue == PassExeQueue::compute)
        {
            
        }

        // fill pass info
        PassInfo_vk passInfo{};
        passInfo.passId = createInfo.passId;
        passInfo.pipeline = pipeline;
        passInfo.writeDepthId = createInfo.writeDepthId;
        passInfo.writeColorIds = writeColorIds;
        passInfo.readImageIds = readImageIds;
        passInfo.rwBufferIds = rwBufferIds;

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


        m_passContainer.addData(passInfo.passId, passInfo);
    }

    void RHIContext_vk::createImage(MemoryReader& _reader)
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
        initPorps.format = getValidFormat(info.format, info.usage, m_imageFormat, m_depthFormat);
        initPorps.usage = getImageUsageFlags(info.usage);
        initPorps.type = getImageType(info.type);
        initPorps.layout = getImageLayout(info.layout);
        initPorps.viewType = getImageViewType( info.viewType);

        std::vector<Image_vk> images;
        vkz::createImage(images, infoList, m_device, m_memProps, initPorps);

        assert(images.size() == info.resNum);

        for (int i = 0; i < info.resNum; ++i)
        {
            m_imageContainer.addData(resArr[i].imgId, images[i]);

            BarrierState_vk barrierState{};
            barrierState.accessMask = getAccessFlags(info.barrierState.access);
            barrierState.stageMask = getPipelineStageFlags(info.barrierState.stage);
            barrierState.imgLayout = initPorps.layout;

            m_imageBarrierStates.addData(resArr[i].imgId, barrierState);
        }
        
        VKZ_DELETE_ARRAY(resArr);
    }

    void RHIContext_vk::createBuffer(MemoryReader& _reader)
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

        for (int i = 0; i < info.resNum; ++i)
        {
            m_bufferContainer.addData(resArr[i].bufId, buffers[i]);

            BarrierState_vk barrierState{};
            barrierState.accessMask = getAccessFlags(info.barrierState.access);
            barrierState.stageMask = getPipelineStageFlags(info.barrierState.stage);

            m_bufferBarrierStates.addData(resArr[i].bufId, barrierState);
        }

        VKZ_DELETE_ARRAY(resArr);
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

    void RHIContext_vk::passRender(uint16_t _passId)
    {
        //const PassInfo_vk& passInfo = m_passInfos[_passId];
        const PassInfo_vk& passInfo = m_passContainer.getData(_passId);

        VkClearColorValue color = { 33.f / 255.f, 200.f / 255.f, 242.f / 255.f, 1 };
        VkClearDepthStencilValue depth = { 0.f, 0 };

        std::vector<VkRenderingAttachmentInfo> colorAttachments(passInfo.writeColorIds.size());
        for (int i = 0; i < passInfo.writeColorIds.size(); ++i)
        {
            const Image_vk& colorTarget = m_imageContainer.getData(passInfo.writeColorIds[i]);

            colorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            colorAttachments[i].clearValue.color = color;
            colorAttachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            colorAttachments[i].imageView = colorTarget.imageView;
        }

        bool hasDepth = passInfo.writeDepthId != kInvalidHandle;
        VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        if (hasDepth)
        {
            const Image_vk& depthTarget = m_imageContainer.getData(passInfo.writeDepthId);

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

        // vkCmdPushDescriptorSetWithTemplateKHR(m_cmdBuffer, passInfo.pipeline, passInfo.pipelineLayout, 0, passInfo.pushConstants.data());
        vkCmdDraw(m_cmdBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(m_cmdBuffer);
    }

} // namespace vkz