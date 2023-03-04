// proj_vk.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <assert.h>
#include <stdio.h>

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include <volk.h>

#include <vector>
#include <algorithm>

#include <fast_obj.h>
#include <meshoptimizer.h>
#include <string>

#define USE_TASK_MESH_SHADER 0 // TODO: task shader not work correctly with the mesh shader

#define USE_FVF 1
#define USE_MESH_SHADER 0



#define VK_CHECK(call) \
	do{ \
		VkResult result = call; \
		assert(result == VK_SUCCESS);\
	} while (0)

VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	createInfo.hinstance = GetModuleHandle(0);
	createInfo.hwnd = glfwGetWin32Window(window);
	VkSurfaceKHR surface;
    VK_CHECK(vkCreateWin32SurfaceKHR(instance, &createInfo, 0, &surface));
	return surface;
#else
#error unsupport platform
#endif
}

uint32_t getGraphicsFamilyIndex(VkPhysicalDevice physicalDevice)
{
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(64);
    uint32_t propertyCount = 64;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertyCount, queueFamilyProperties.data());

    for (uint32_t i = 0; i < propertyCount; ++i)
    {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            return i;
    }

    return VK_QUEUE_FAMILY_IGNORED;
}

bool supportPresentation(VkPhysicalDevice physicalDevice, uint32_t familyIndex)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, familyIndex);
#else
    return true;
#endif
}

VkPhysicalDevice pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDevicesCount)
{
    VkPhysicalDevice discrete = 0;
    VkPhysicalDevice fallback = 0;

	for (uint32_t i = 0; i < physicalDevicesCount; ++i)
	{
        VkPhysicalDeviceProperties2 props = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        vkGetPhysicalDeviceProperties2(physicalDevices[i], &props);

        printf("GPU%d: %s\n", i, props.properties.deviceName);

        uint32_t familyIndex = getGraphicsFamilyIndex(physicalDevices[i]);

        if(familyIndex == VK_QUEUE_FAMILY_IGNORED)
            continue;

        if (!supportPresentation(physicalDevices[i], familyIndex))
            continue;

        if (!discrete && props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            discrete = physicalDevices[i];
        }

        if (!fallback) {
            fallback = physicalDevices[i];
        }
	}

    VkPhysicalDevice result = discrete ? discrete : fallback;

    if(result) { 
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(result, &props);

        printf("Selected GPU %s\n", props.deviceName);
    }
    else {
        printf("ERROR: No GPU Found!\n");
    }


    return result;
}


VkBool32 debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    const char* type =
        flags & VK_DEBUG_REPORT_ERROR_BIT_EXT
        ? "ERROR"
        : (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
            ? "WARNING"
            : "INFO";
        
    char message[4096];
    snprintf(message, ARRAYSIZE(message), "%s: %s\n", type, pMessage);

    printf("%s\n", message);

#ifdef _WIN32
    OutputDebugStringA(message);
#endif

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        assert(!"validation error encountered!");
    }
   return VK_FALSE;

}


VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance)
{
    VkDebugReportCallbackCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
    createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
    createInfo.pfnCallback = &debugReportCallback;

    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

    VkDebugReportCallbackEXT callback = 0;
    VK_CHECK(vkCreateDebugReportCallbackEXT(instance, &createInfo, 0, &callback));

    return callback;
}

VkInstance createInstance()
{

    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &appInfo;

#ifdef _DEBUG
    const char* debugLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    createInfo.ppEnabledLayerNames = debugLayers;
    createInfo.enabledLayerCount = sizeof(debugLayers) / sizeof(debugLayers[0]);
#endif

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif // VK_USE_PLATFORM_WIN32_KHR
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);


    VkInstance Instance = 0;
    VK_CHECK(vkCreateInstance(&createInfo, 0, &Instance));

	return Instance;
}


VkDevice createDevice(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t familyIndex)
{
    float queueProps[] = { 1.0f };

    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = familyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = queueProps;

	const char* extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
        VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
        VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
#if USE_MESH_SHADER
        VK_EXT_MESH_SHADER_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME, // required by VK_EXT_MESH_SHADER_EXTENSION_NAME
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME, //required by VK_KHR_SPIRV_1_4_EXTENSION_NAME
#endif
	};

    VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    features.features.vertexPipelineStoresAndAtomics = true;

    VkPhysicalDevice8BitStorageFeatures features8 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES };
    features8.storageBuffer8BitAccess = true;
    features8.uniformAndStorageBuffer8BitAccess = true; // TODO: this is seems something weird that SPIV-R automatic enabled the Access in assembly. need find a way to fix it.

    VkPhysicalDevice16BitStorageFeatures features16 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES };
    features16.storageBuffer16BitAccess = true;

#if USE_MESH_SHADER
    VkPhysicalDeviceMeshShaderFeaturesEXT featuresMesh = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
    featuresMesh.meshShader = true;
#if USE_TASK_MESH_SHADER
    featuresMesh.taskShader = true;
#endif
#endif

    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);

    createInfo.pNext = &features;
    features.pNext = &features16;
    features16.pNext = &features8;
#if USE_MESH_SHADER
    features8.pNext = &featuresMesh;
#endif

    VkDevice device = 0;

    VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, 0, &device));

	return device;
}

VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{  
    std::vector<VkSurfaceFormatKHR> formats(32);
    uint32_t formatCount = 32;

    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data()));

    if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        return VK_FORMAT_R8G8B8A8_UNORM;
    }

    for (uint32_t i = 0; i < formatCount; ++i) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM || formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
            return formats[i].format;
        }
    }

    return formats[0].format;
}


VkSwapchainKHR createSwapchain(VkDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR surfaceCaps, uint32_t* familyIndex, VkFormat format
    , VkSwapchainKHR oldSwapchain)
{

    uint32_t width = surfaceCaps.currentExtent.width;
    uint32_t height = surfaceCaps.currentExtent.height;

    VkCompositeAlphaFlagBitsKHR surfaceComposite =
        (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
        : (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
        : (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
        : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;


    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = surface;
    createInfo.minImageCount = std::max(2u, surfaceCaps.minImageCount);   
    createInfo.imageFormat = format;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent.width = width;
    createInfo.imageExtent.height = height;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = familyIndex;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = surfaceComposite;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.oldSwapchain = oldSwapchain;

    VkSwapchainKHR swapchain = 0;
    VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain));

    return swapchain;
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

    VkCommandPool commandPool = 0;
    VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &commandPool));

    return commandPool;
}

VkRenderPass createRenderPass(VkDevice device, VkFormat format)
{
    VkAttachmentDescription attachments[1] = {};
    attachments[0].format = format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkAttachmentReference colorAttachments = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachments;

    VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    createInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;

    VkRenderPass renderPass = 0;
    VK_CHECK(vkCreateRenderPass(device, &createInfo, 0, &renderPass));

    return renderPass;
}

VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView imageView, uint32_t width, uint32_t height)
{
    VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &imageView;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = 1;

    VkFramebuffer framebuffer = 0;

    VK_CHECK(vkCreateFramebuffer(device, &createInfo, 0, &framebuffer));

    return framebuffer;
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format)
{
    VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView = 0;
    VK_CHECK(vkCreateImageView(device, &createInfo, 0, &imageView));
    return imageView;
}

VkShaderModule loadShader(VkDevice device, const char* path)
{
    FILE* file = fopen(path, "rb");
    assert(file);

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    assert(length >= 0);
    fseek(file, 0, SEEK_SET);

    char* buffer = new char[length];
    assert(buffer);

    size_t rc = fread(buffer, 1, length, file);
    assert(rc = size_t(length));
    fclose(file);

    assert(length % 4 == 0);

    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);
    createInfo.codeSize = length;

    VkShaderModule shaderModule = 0;
    VK_CHECK(vkCreateShaderModule(device, &createInfo, 0, &shaderModule));

    return shaderModule;
}

VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout& outSetLayout)
{
#if USE_MESH_SHADER
    VkDescriptorSetLayoutBinding setBindings[2] = {};
    setBindings[0].binding = 0;
    setBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    setBindings[0].descriptorCount = 1;
    setBindings[0].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;

    setBindings[1].binding = 1;
    setBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    setBindings[1].descriptorCount = 1;
    setBindings[1].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
#else
    VkDescriptorSetLayoutBinding setBindings[1] = {};
    setBindings[0].binding = 0;
    setBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    setBindings[0].descriptorCount = 1;
    setBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
#endif

    VkDescriptorSetLayoutCreateInfo setCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    setCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;

#if !USE_FVF
    setCreateInfo.bindingCount = ARRAYSIZE(setBindings);
    setCreateInfo.pBindings = setBindings;
#endif

    VkDescriptorSetLayout setLayout = 0;

    VK_CHECK(vkCreateDescriptorSetLayout(device, &setCreateInfo, 0, &setLayout));
    VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &setLayout;

    VkPipelineLayout layout = 0;
    VK_CHECK(vkCreatePipelineLayout(device, &createInfo, 0, &layout));

    outSetLayout = setLayout;
    return layout;
}


struct Vertex
{
    float vx, vy, vz;
    uint8_t nx, ny, nz, nw;
    float tu, tv;
};


VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, VkRenderPass renderPass, VkShaderModule VS, VkShaderModule FS, VkShaderModule TS, VkShaderModule MS)
{
    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

#if USE_MESH_SHADER && USE_TASK_MESH_SHADER
    VkPipelineShaderStageCreateInfo stages[3] = {};

    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_TASK_BIT_EXT;
    stages[0].module = TS;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_MESH_BIT_EXT;
    stages[1].module = MS;
    stages[1].pName = "main";

    stages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[2].module = FS;
    stages[2].pName = "main";

#elif USE_MESH_SHADER && !USE_TASK_MESH_SHADER
    VkPipelineShaderStageCreateInfo stages[2] = {};

    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_MESH_BIT_EXT;
    stages[0].module = MS;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = FS;
    stages[1].pName = "main";

#else
    VkPipelineShaderStageCreateInfo stages[2] = {};

    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = VS;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = FS;
    stages[1].pName = "main";
#endif

    createInfo.stageCount = sizeof(stages) / sizeof(stages[0]);
    createInfo.pStages = stages;


    VkPipelineVertexInputStateCreateInfo vertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    createInfo.pVertexInputState = &vertexInput;

#if USE_FVF
    VkVertexInputBindingDescription fvfBindingDescs[1] = {};
    fvfBindingDescs[0].stride = sizeof(Vertex);


    VkVertexInputAttributeDescription fvfAttrDescs[3] = {};
    fvfAttrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    fvfAttrDescs[0].location = 0;
    fvfAttrDescs[0].offset = offsetof(Vertex, vx);
    fvfAttrDescs[1].format = VK_FORMAT_R8G8B8A8_UINT;
    fvfAttrDescs[1].location = 1;
    fvfAttrDescs[1].offset = offsetof(Vertex, nx);
    fvfAttrDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
    fvfAttrDescs[2].location = 2;
    fvfAttrDescs[2].offset = offsetof(Vertex, tu);


    vertexInput.vertexBindingDescriptionCount = ARRAYSIZE(fvfBindingDescs);
    vertexInput.pVertexBindingDescriptions = fvfBindingDescs;

    vertexInput.vertexAttributeDescriptionCount = ARRAYSIZE(fvfAttrDescs);
    vertexInput.pVertexAttributeDescriptions = fvfAttrDescs;
#endif

    VkPipelineInputAssemblyStateCreateInfo inputAssemply = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssemply.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    createInfo.pInputAssemblyState = &inputAssemply;

    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    createInfo.pViewportState = &viewportState;

    VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizationState.lineWidth = 1.f;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    createInfo.pRasterizationState = &rasterizationState;

    VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.pMultisampleState = &multisampleState;
    
    VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    createInfo.pDepthStencilState = &depthStencilState;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;
    createInfo.pColorBlendState = &colorBlendState;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
    dynamicState.pDynamicStates = dynamicStates;
    createInfo.pDynamicState = &dynamicState;

    createInfo.layout = layout;
    createInfo.renderPass = renderPass;

    VkPipeline pipeline = 0;


    VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, 0, &pipeline));

    return pipeline;
}



VkImageMemoryBarrier imageBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout,  VkImageLayout newLayout)
{   
    VkImageMemoryBarrier result = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

    result.srcAccessMask = srcAccessMask;
    result.dstAccessMask = dstAccessMask;
    result.oldLayout = oldLayout;
    result.newLayout = newLayout;
    result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.image = image;
    result.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return result;
}

struct Swapchain
{
    VkSwapchainKHR swapchain;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;

    
    uint32_t width, height;
    uint32_t imageCount;
};

void createSwapchain(Swapchain& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t* familyIndex
    , VkFormat format, VkRenderPass renderPass, VkSwapchainKHR oldSwapchain = 0)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    uint32_t width = surfaceCaps.currentExtent.width;
    uint32_t height= surfaceCaps.currentExtent.height;

    VkSwapchainKHR swapchain = createSwapchain(device, surface, surfaceCaps, familyIndex, format, oldSwapchain);
    assert(swapchain);

    std::vector<VkImage> images(16);
    uint32_t imageCount = 16;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data()));

    std::vector<VkImageView> imageViews(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i) {
        imageViews[i] = createImageView(device, images[i], format);
        assert(imageViews[i]);
    }

    std::vector <VkFramebuffer> framebuffers(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i) {
        framebuffers[i] = createFramebuffer(device, renderPass, imageViews[i], width, height);
        assert(framebuffers[i]);
    }

    result.swapchain = swapchain;

    result.images = images;
    result.imageViews = imageViews;
    result.framebuffers = framebuffers;

    result.width = width;
    result.height = height;
    result.imageCount = imageCount;
}

void destroySwapchain(VkDevice device, const Swapchain& swapchain)
{
    for (uint32_t i = 0; i < swapchain.imageCount; ++i) {
        vkDestroyFramebuffer(device, swapchain.framebuffers[i], 0);
    }

    for (uint32_t i = 0; i < swapchain.imageCount; ++i) {
        vkDestroyImageView(device, swapchain.imageViews[i], 0);
    }

    vkDestroySwapchainKHR(device, swapchain.swapchain, 0);
}

void resizeSwapchainIfNecessary(Swapchain& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t* familyIndex
    , VkFormat format, VkRenderPass renderPass)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    if (result.width == surfaceCaps.currentExtent.width && result.height == surfaceCaps.currentExtent.height)
    {
        return;
    }

    Swapchain old = result;
    createSwapchain(result, physicalDevice, device, surface, familyIndex, format, renderPass, old.swapchain);
    

    VK_CHECK(vkDeviceWaitIdle(device));
    destroySwapchain(device, old);
}

struct Meshlet
{
    uint32_t vertices[64];
    uint8_t indices[126]; //this is 42 triangles
    uint8_t indexCount;
    uint8_t vertexCount;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Meshlet> meshlets;
};

struct Buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* data;
    size_t size;
};

uint32_t selectMemoryType(const VkPhysicalDeviceMemoryProperties& memoryProps, uint32_t memoryTypeBits, VkMemoryPropertyFlags flags)
{
    for (uint32_t i = 0; i < memoryProps.memoryTypeCount; ++i) {
        if ( (memoryTypeBits & (1 << i)) != 0 && (memoryProps.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    assert(!"No compatible memory type found!\n");
    return ~0u;
}

void createBuffer(Buffer& result, const VkPhysicalDeviceMemoryProperties memoryProps, VkDevice device, size_t sz, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = sz;
    createInfo.usage = usage;

    VkBuffer buffer = 0;
    VK_CHECK(vkCreateBuffer(device, &createInfo, 0, &buffer));

    VkMemoryRequirements memoryReqs;
    vkGetBufferMemoryRequirements(device, buffer, &memoryReqs);

    uint32_t memoryTypeIdx = selectMemoryType(memoryProps, memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    assert(memoryTypeIdx != ~0u);

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memoryReqs.size;
    allocInfo.memoryTypeIndex = memoryTypeIdx;
    
    VkDeviceMemory memory = 0;
    VK_CHECK(vkAllocateMemory(device, &allocInfo, 0, &memory));

    VK_CHECK(vkBindBufferMemory(device, buffer, memory, 0));

    void* data = 0;
    VK_CHECK(vkMapMemory(device, memory, 0, sz, 0, &data));

    result.buffer = buffer;
    result.memory = memory;
    result.data = data;
    result.size = sz;
}

void destroyBuffer(const Buffer& buffer, VkDevice device)
{
    // depends on the doc:
    //    If a memory object is mapped at the time it is freed, it is implicitly unmapped.
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkFreeMemory.html
    vkFreeMemory(device, buffer.memory, 0);
    vkDestroyBuffer(device, buffer.buffer, 0);
}


bool checkExtSupportness(std::vector<VkExtensionProperties>& props, const char* extName)
{
    bool extSupported = false;
    for (const auto& extension : props) {
        if (std::string(extension.extensionName) == extName) {
            extSupported = true;
            break;
        }
    }

    printf("%s : %s\n", extName, extSupported ? "true" : "false");

    return extSupported;
}

VkQueryPool createQueryPool(VkDevice device, uint32_t queryCount)
{
    VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = queryCount;

    VkQueryPool queryPool = nullptr;
    VK_CHECK(vkCreateQueryPool(device, &createInfo, 0, &queryPool));
    return queryPool;
}

void buildMeshlets(Mesh& mesh)
{
    Meshlet meshlet = {};
    std::vector<uint8_t> meshletVertices(mesh.vertices.size(), (uint8_t)0xff);

    for (size_t i = 0; i < mesh.indices.size(); i += 3)
    {
        uint32_t a = mesh.indices[i + 0];
        uint32_t b = mesh.indices[i + 1];
        uint32_t c = mesh.indices[i + 2];

        uint8_t& av = meshletVertices[a];
        uint8_t& bv = meshletVertices[b];
        uint8_t& cv = meshletVertices[c];
        if (meshlet.vertexCount + (av == 0xff) + (bv == 0xff) + (cv == 0xff) > 64
            || meshlet.indexCount + 3 > 126) {
            mesh.meshlets.push_back(meshlet);


            for (size_t j = 0; j < meshlet.vertexCount; ++j)
            {
                meshletVertices[meshlet.vertices[j]] = (uint8_t)0xff;
            }

            meshlet = {};
        }

        if (av == 0xff)
        {
            av = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = a;
        }
        if (bv == 0xff)
        {
            bv = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = b;
        }
        if (cv == 0xff)
        {
            cv = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = c;
        }

        meshlet.indices[meshlet.indexCount++] = av;
        meshlet.indices[meshlet.indexCount++] = bv;
        meshlet.indices[meshlet.indexCount++] = cv;
        
    }

    if (meshlet.indexCount)
    {
        mesh.meshlets.push_back(meshlet);
    }

}

void dumpExtensionSupport(VkPhysicalDevice physicalDevice)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    checkExtSupportness(availableExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_8BIT_STORAGE_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_EXT_MESH_SHADER_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_SPIRV_1_4_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
}

bool loadMesh(Mesh& result, const char* path)
{
    fastObjMesh* obj =  fast_obj_read(path);
    if (!obj)
        return false;

    size_t index_count = obj->index_count;

    std::vector<Vertex> triangle_vertices;
    triangle_vertices.resize(index_count);

    size_t vertex_offset = 0, index_offset = 0;

    for (uint32_t i = 0; i < obj->face_count; ++i)
    {
        for (uint32_t j = 0; j < obj->face_vertices[i]; ++j)
        {
            fastObjIndex gi = obj->indices[index_offset + j];

            assert(j < 3);

            Vertex& v = triangle_vertices[vertex_offset++];

            v.vx = obj->positions[gi.p * 3 + 0] * 0.008f;
            v.vy = obj->positions[gi.p * 3 + 1] * 0.008f;
            v.vz = obj->positions[gi.p * 3 + 2] * 0.008f;

            v.nx = uint8_t(obj->normals[gi.n * 3 + 0] * 127.f + 127.5f);
            v.ny = uint8_t(obj->normals[gi.n * 3 + 1] * 127.f + 127.5f);
            v.nz = uint8_t(obj->normals[gi.n * 3 + 2] * 127.f + 127.5f);

            v.tu = meshopt_quantizeHalf( obj->texcoords[gi.t * 2 + 0]);
            v.tv = meshopt_quantizeHalf( obj->texcoords[gi.t * 2 + 1]);
        }

        index_offset += obj->face_vertices[i];
    }

    assert(vertex_offset == index_count);

    fast_obj_destroy(obj);

    std::vector<uint32_t> remap(index_count);
    size_t vertex_count = meshopt_generateVertexRemap(remap.data(), 0, index_count, triangle_vertices.data(), index_count, sizeof(Vertex));

    std::vector<Vertex> vertices(vertex_count);
    std::vector<uint32_t> indices(index_count);


    meshopt_remapVertexBuffer(vertices.data(), triangle_vertices.data(), index_count, sizeof(Vertex), remap.data());
    meshopt_remapIndexBuffer(indices.data(), 0, index_count, remap.data());

    meshopt_optimizeVertexCache(indices.data(), indices.data(), index_count, vertex_count);
    meshopt_optimizeVertexFetch(vertices.data(), indices.data(), index_count, vertices.data(), vertex_count, sizeof(Vertex));

    result.vertices.insert(result.vertices.end(), vertices.begin(), vertices.end());
    result.indices.insert(result.indices.end(), indices.begin(), indices.end());

    return true;
}

int main(int argc, const char** argv)
{
    if (argc < 2) {
        printf("Usage: %s [mesh]\n", argv[0]);
        return 1;
    }

	int rc = glfwInit();
	assert(rc);

    VK_CHECK(volkInitialize());

	VkInstance instance = createInstance();
	assert(instance);

    volkLoadInstance(instance);

    VkDebugReportCallbackEXT debugCallback = registerDebugCallback(instance);

	VkPhysicalDevice physicalDevices[16];
	uint32_t deviceCount = sizeof(physicalDevices) / sizeof(physicalDevices[0]);
	VK_CHECK( vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices));


	VkPhysicalDevice physicalDevice = pickPhysicalDevice(physicalDevices, deviceCount);
	assert(physicalDevice);

    dumpExtensionSupport(physicalDevice);

    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    assert(props.limits.timestampPeriod);

    uint32_t familyIndex = getGraphicsFamilyIndex(physicalDevice);
    assert(familyIndex != VK_QUEUE_FAMILY_IGNORED);

	VkDevice device = createDevice(instance, physicalDevice, familyIndex);
	assert(device);

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "proj_vk", 0, 0);
	assert(window);

	VkSurfaceKHR surface = createSurface(instance, window);
	assert(surface);

    VkBool32 presentSupported = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &presentSupported));
    assert(presentSupported);

    VkFormat swapchainFormat = getSwapchainFormat(physicalDevice, surface);
    assert(swapchainFormat);
    
    VkSemaphore acquirSemaphore = createSemaphore(device);
    assert(acquirSemaphore);

    VkSemaphore releaseSemaphore = createSemaphore(device);
    assert(releaseSemaphore);

    VkQueue queue = 0;
    vkGetDeviceQueue(device, familyIndex, 0, &queue);
    assert(queue);

    VkRenderPass renderPass = createRenderPass(device, swapchainFormat);
    assert(renderPass);

#if USE_FVF
    VkShaderModule meshVS = loadShader(device, "shaders/meshfvf.vert.spv");
    assert(meshVS);
#else
    VkShaderModule meshVS = loadShader(device, "shaders/mesh.vert.spv");
    assert(meshVS);
#endif

    VkShaderModule meshFS = loadShader(device, "shaders/mesh.frag.spv");
    assert(meshFS);

    VkShaderModule meshletMS = 0;
#if USE_MESH_SHADER
    meshletMS = loadShader(device, "shaders/meshlet.mesh.spv");
    assert(meshletMS);
#endif

    VkShaderModule meshletTS = 0;
#if USE_TASK_MESH_SHADER
    meshletTS = loadShader(device, "shaders/meshlet.task.spv");
    assert(meshletTS);
#endif

    VkDescriptorSetLayout descSetLayout = 0;
    VkPipelineLayout meshLayout = createPipelineLayout(device, descSetLayout);
    assert(meshLayout);
    assert(descSetLayout);

    VkPipelineCache pipelineCache = 0;
    VkPipeline pipeline = createGraphicsPipeline(device, pipelineCache, meshLayout,renderPass, meshVS, meshFS, meshletTS, meshletMS);
    assert(pipeline);

    Swapchain swapchain = {};
    createSwapchain(swapchain, physicalDevice, device, surface, &familyIndex, swapchainFormat, renderPass);

    VkQueryPool queryPool = createQueryPool(device, 128);
    assert(queryPool);

    VkCommandPool commandPool = createCommandPool(device, familyIndex);
    assert(commandPool);

    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuffer = 0;
    VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &cmdBuffer));

    
    VkPhysicalDeviceMemoryProperties memoryProps = {};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProps);
    
    Mesh mesh;
    bool rcm = loadMesh(mesh, argv[1]);
    assert(rcm);
#if USE_MESH_SHADER
    buildMeshlets(mesh);
#endif
    Buffer vb = {};
#if USE_FVF
    createBuffer(vb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
#else
    createBuffer(vb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
#endif

    Buffer ib = {};
    createBuffer(ib, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

#if USE_MESH_SHADER
    Buffer mb = {};
    createBuffer(mb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
#endif

    assert(vb.data && vb.size >= mesh.vertices.size() * sizeof(Vertex));
    memcpy(vb.data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    
    assert(ib.data&& ib.size >= mesh.indices.size() * sizeof(uint32_t));
    memcpy(ib.data, mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t));

#if USE_MESH_SHADER
    assert(mb.data&& mb.size >= mesh.meshlets.size() * sizeof(Meshlet));
    memcpy(mb.data, mesh.meshlets.data(), mesh.meshlets.size() * sizeof(Meshlet));
#endif

    while (!glfwWindowShouldClose(window)) {

        double frameCpuBegin = glfwGetTime() * 1000.0;

        glfwPollEvents();

        int newWindowWidth = 0, newWindowHeight = 0;
        glfwGetWindowSize(window, &newWindowWidth, &newWindowHeight);


        resizeSwapchainIfNecessary(swapchain, physicalDevice, device, surface, &familyIndex, swapchainFormat, renderPass);


        uint32_t imageIndex = 0;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain.swapchain, ~0ull, acquirSemaphore, VK_NULL_HANDLE, &imageIndex));

        VK_CHECK(vkResetCommandPool(device, commandPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

        vkCmdResetQueryPool(cmdBuffer, queryPool, 0, 128);
        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 0);

        
        VkImageMemoryBarrier renderBeginBarrier = imageBarrier(swapchain.images[imageIndex], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
            , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // check https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#combined-graphicspresent-queue
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            , VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &renderBeginBarrier);
        

        VkClearColorValue color = { 33.f/255.f, 200.f/255.f, 242.f/255.f, 1 };
        VkClearValue clearColor = { color };

        VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        passBeginInfo.renderPass = renderPass;
        passBeginInfo.framebuffer = swapchain.framebuffers[imageIndex];
        passBeginInfo.renderArea.extent.width = swapchain.width;
        passBeginInfo.renderArea.extent.height = swapchain.height;
        passBeginInfo.clearValueCount = 1;
        passBeginInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(cmdBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // drawcalls
        VkViewport viewport = { 0.f, float(swapchain.height), float(swapchain.width), -float(swapchain.height), 0.f, 1.f };
        VkRect2D scissor = { {0, 0}, {uint32_t(swapchain.width), uint32_t(swapchain.height)}};

        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);


#if USE_FVF
        VkDeviceSize vbOffset = 0;
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vb.buffer, &vbOffset);
        vkCmdBindIndexBuffer(cmdBuffer, ib.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmdBuffer, (uint32_t)mesh.indices.size(), 1, 0, 0, 0);

#elif USE_MESH_SHADER
        VkDescriptorBufferInfo vbInfo = { };
        vbInfo.buffer = vb.buffer;
        vbInfo.offset = 0;
        vbInfo.range = vb.size;

        VkDescriptorBufferInfo mbInfo = { };
        mbInfo.buffer = mb.buffer;
        mbInfo.offset = 0;
        mbInfo.range = mb.size;

        VkWriteDescriptorSet descSets[2] = {};

        descSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descSets[0].dstBinding = 0;
        descSets[0].descriptorCount = 1;
        descSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descSets[0].pBufferInfo = &vbInfo;


        descSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descSets[1].dstBinding = 1;
        descSets[1].descriptorCount = 1;
        descSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descSets[1].pBufferInfo = &mbInfo;

        vkCmdPushDescriptorSetKHR(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshLayout, 0, ARRAYSIZE(descSets), descSets);

        vkCmdDrawMeshTasksEXT(cmdBuffer, (uint32_t)mesh.meshlets.size(), 1, 1);
#else
        VkDescriptorBufferInfo vbInfo = { };
        vbInfo.buffer = vb.buffer;
        vbInfo.offset = 0;
        vbInfo.range = vb.size;

        VkWriteDescriptorSet descSets[1] = {};

        descSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descSets[0].dstBinding = 0;
        descSets[0].descriptorCount = 1;
        descSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descSets[0].pBufferInfo = &vbInfo;

        vkCmdPushDescriptorSetKHR(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshLayout, 0, ARRAYSIZE(descSets), descSets);

        vkCmdBindIndexBuffer(cmdBuffer, ib.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmdBuffer, (uint32_t)mesh.indices.size(), 1, 0, 0, 0);

#endif
        vkCmdEndRenderPass(cmdBuffer);

        VkImageMemoryBarrier renderEndBarrier = imageBarrier(swapchain.images[imageIndex], VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0
            , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
            , VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &renderEndBarrier);

        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 1);

        VK_CHECK(vkEndCommandBuffer(cmdBuffer));

 
        VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &acquirSemaphore;
        submitInfo.pWaitDstStageMask = &submitStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &releaseSemaphore;

        VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pWaitSemaphores = &releaseSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));

        double waitTimeBegin = glfwGetTime() * 1000.0;

        VK_CHECK(vkDeviceWaitIdle(device));
        
        double waitTimeEnd = glfwGetTime() * 1000.0;
        
        uint64_t queryResults[2] = {};
        vkGetQueryPoolResults(device, queryPool, 0, ARRAYSIZE(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT);

        double frameCpuEnd = glfwGetTime() * 1000.0;

        double frameGpuBegin = double(queryResults[0]) * props.limits.timestampPeriod * 1e-6;
        double frameGpuEnd = double(queryResults[1]) * props.limits.timestampPeriod * 1e-6;
        
        char title[256];
        sprintf(title, "cpu: %.1f ms; gpu %.1f ms; wait %.2f ms; primitive: %zu; meshlets: %zu"
            , frameCpuEnd - frameCpuBegin, frameGpuEnd - frameGpuBegin, waitTimeEnd - waitTimeBegin
            , mesh.indices.size() / 3, mesh.meshlets.size());
        glfwSetWindowTitle(window, title);
                
	}
    VK_CHECK(vkDeviceWaitIdle(device));


    destroyBuffer(ib, device); 
    destroyBuffer(vb, device);
#if USE_MESH_SHADER
    destroyBuffer(mb, device);
#endif
    


    vkDestroyCommandPool(device, commandPool, 0);

    destroySwapchain(device, swapchain);

    vkDestroyQueryPool(device, queryPool, 0);

    vkDestroyPipeline(device, pipeline, 0);
    vkDestroyDescriptorSetLayout(device, descSetLayout, 0);
    vkDestroyPipelineLayout(device, meshLayout, 0);

    vkDestroyShaderModule(device, meshFS, 0);
    vkDestroyShaderModule(device, meshVS, 0);
#if USE_MESH_SHADER
    vkDestroyShaderModule(device, meshletMS, 0);
#endif

#if USE_TASK_MESH_SHADER
    vkDestroyShaderModule(device, meshletTS, 0);
#endif

    vkDestroyRenderPass(device, renderPass, 0);

    vkDestroySemaphore(device, releaseSemaphore, 0);
    vkDestroySemaphore(device, acquirSemaphore, 0);
    vkDestroySurfaceKHR(instance, surface, 0);

	glfwDestroyWindow(window);

    
    vkDestroyDevice(device, 0);

    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT(instance, debugCallback, 0);


    vkDestroyInstance(instance, 0);
}
