#include "common.h"
#include "device.h"

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

static VkBool32 debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    const char* type =
        flags & VK_DEBUG_REPORT_ERROR_BIT_EXT
        ? "ERROR"
        : (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
        ? "WARNING"
        : "INFO";

    char message[4096];
    snprintf(message, COUNTOF(message), "%s: %s\n", type, pMessage);

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

        if (familyIndex == VK_QUEUE_FAMILY_IGNORED)
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

    if (result) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(result, &props);

        printf("Selected GPU %s\n", props.deviceName);
    }
    else {
        printf("ERROR: No GPU Found!\n");
    }


    return result;
}


VkDevice createDevice(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t familyIndex, bool meshShadingSupported)
{
    float queueProps[] = { 1.0f };

    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = familyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = queueProps;

    std::vector<const char*> extensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
        VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
        VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
    };

    if (meshShadingSupported)
    {
        extensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
        extensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
        extensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
    }

    VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    features.features.vertexPipelineStoresAndAtomics = true;

    VkPhysicalDevice8BitStorageFeatures features8 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES };
    features8.storageBuffer8BitAccess = true;
    features8.uniformAndStorageBuffer8BitAccess = true; // TODO: this is seems something weird that SPIV-R automatic enabled the Access in assembly. need find a way to fix it.

    VkPhysicalDevice16BitStorageFeatures features16 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES };
    features16.storageBuffer16BitAccess = true;
    features16.uniformAndStorageBuffer16BitAccess = true;

    VkPhysicalDeviceMeshShaderFeaturesEXT featuresMesh = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
    featuresMesh.meshShader = true;
    featuresMesh.taskShader = false; // TODO: not using it yet


    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = uint32_t(extensions.size());

    createInfo.pNext = &features;
    features.pNext = &features16;
    features16.pNext = &features8;
    if (meshShadingSupported)
        features8.pNext = &featuresMesh;


    VkDevice device = 0;

    VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, 0, &device));

    return device;
}
