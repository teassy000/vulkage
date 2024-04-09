#pragma once

#include "common.h"

namespace kage
{
    VkInstance createInstance();
    VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance);

    uint32_t getGraphicsFamilyIndex(VkPhysicalDevice physicalDevice);

    VkPhysicalDevice pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDevicesCount);

    VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance);

    VkDevice createDevice(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t familyIndex, bool meshShadingSupported);
} // namespace kage
