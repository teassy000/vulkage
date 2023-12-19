#ifndef __VKZ_VK_DEVICE_H__
#define __VKZ_VK_DEVICE_H__

#include "common.h"

namespace vkz
{
    VkInstance createInstance();
    VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance);

    uint32_t getGraphicsFamilyIndex(VkPhysicalDevice physicalDevice);

    VkPhysicalDevice pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDevicesCount);

    VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance);

    VkDevice createDevice(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t familyIndex, bool meshShadingSupported);
} // namespace vkz

#endif // __VKZ_VK_DEVICE_H__