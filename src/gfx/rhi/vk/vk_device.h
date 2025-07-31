#pragma once

#include "core/common.h"
#include "kage_rhi_vk.h"

namespace kage { namespace vk
{
    VkInstance createInstance();
    VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance);

    uint32_t getGraphicsFamilyIndex(VkPhysicalDevice physicalDevice);

    VkPhysicalDevice pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDevicesCount);

    VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance);

    VkDevice createDevice(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t familyIndex, bool meshShadingSupported);

}
} // namespace kage
