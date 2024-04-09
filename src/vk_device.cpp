#include "common.h"
#include "macro.h"
#include "vk_device.h"

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace kage
{

    static VkBool32 debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
    {
        DebugMessageType msgType = flags & VK_DEBUG_REPORT_ERROR_BIT_EXT
            ? DebugMessageType::error
            : (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
            ? DebugMessageType::warning
            : DebugMessageType::info;

        kage::message(msgType, "%s\n", pMessage);

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
        assert(volkGetInstanceVersion() >= VK_API_VERSION_1_3);

        VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        createInfo.pApplicationInfo = &appInfo;

#ifdef _DEBUG
        const char* debugLayers[] = {
            "VK_LAYER_KHRONOS_validation",
        };
        createInfo.ppEnabledLayerNames = debugLayers;
        createInfo.enabledLayerCount = sizeof(debugLayers) / sizeof(debugLayers[0]);


        VkValidationFeatureEnableEXT enabledValidationFeatures[] = {
            VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
        };

        VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
        validationFeatures.enabledValidationFeatureCount = sizeof(enabledValidationFeatures) / sizeof(enabledValidationFeatures[0]);
        validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

        createInfo.pNext = &validationFeatures;

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
        uint32_t propertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertyCount, nullptr);
        stl::vector<VkQueueFamilyProperties> queueFamilyProperties(propertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertyCount, queueFamilyProperties.data());

        VkQueueFlags requiredFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

        for (uint32_t i = 0; i < propertyCount; ++i)
        {
            if (queueFamilyProperties[i].queueFlags & requiredFlags)
            {
                return i;
            }
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
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

            kage::message(kage::info, "GPU%d: %s\n", i, props.deviceName);

            uint32_t familyIndex = getGraphicsFamilyIndex(physicalDevices[i]);

            if (familyIndex == VK_QUEUE_FAMILY_IGNORED)
                continue;

            if (!supportPresentation(physicalDevices[i], familyIndex))
                continue;

            if (!discrete && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
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

            kage::message(kage::info, "Selected GPU %s\n", props.deviceName);
        }
        else {
            kage::message(kage::info, "ERROR: No GPU Found!\n");
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

        stl::vector<const char*> extensions;
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        extensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);

        if (meshShadingSupported)
        {
            extensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
        }

        extensions.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);


        VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        features.features.vertexPipelineStoresAndAtomics = true;
        features.features.multiDrawIndirect = true;
        features.features.pipelineStatisticsQuery = true;
        features.features.shaderInt16 = true;
        features.features.shaderInt64 = true;

        VkPhysicalDeviceVulkan11Features features11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        features11.storageBuffer16BitAccess = true;
        features11.uniformAndStorageBuffer16BitAccess = true;
        features11.shaderDrawParameters = true;

        VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        features12.drawIndirectCount = true;
        features12.storageBuffer8BitAccess = true;
        features12.uniformAndStorageBuffer8BitAccess = true;
        features12.samplerFilterMinmax = true;
        features12.shaderFloat16 = true;
        features12.shaderInt8 = true;

        VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        features13.dynamicRendering = true; // for vkCmdBeginRendering
        features13.synchronization2 = true;

        VkPhysicalDeviceMeshShaderFeaturesEXT featuresMesh = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
        featuresMesh.meshShader = true;
        featuresMesh.taskShader = true;

        VkPhysicalDeviceFragmentShadingRateFeaturesKHR featuresFSR = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR };
        featuresFSR.pipelineFragmentShadingRate = true;

        VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = &queueInfo;
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledExtensionCount = uint32_t(extensions.size());

        createInfo.pNext = &features;
        features.pNext = &features11;
        features11.pNext = &features12;
        features12.pNext = &features13;
        features13.pNext = &featuresFSR;

        if (meshShadingSupported)
            featuresFSR.pNext = &featuresMesh;


        VkDevice device = 0;

        VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, 0, &device));

        return device;
    }
} // namespace kage