#include "common.h"
#include "vk_swapchain.h"

#include "macro.h"

#include <glm/common.hpp> // for glm::max

#include "rhi_context_vk.h"

#include <windows.h>


namespace kage { namespace vk
{
    extern RHIContext_vk* s_renderVK;

    VkPresentModeKHR getSwapchainPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        stl::vector<VkPresentModeKHR> presentModes(32);
        uint32_t presentModeCount = 32;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

        for (uint32_t i = 0; i < presentModeCount; ++i) {
            if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                return presentModes[i];
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkResult Swapchain_vk::create(void* _nwh)
    {
        const VkPhysicalDevice physicalDevice = s_renderVK->m_physicalDevice;
        const uint32_t queueFamilyIdx = s_renderVK->m_gfxFamilyIdx;
        
        m_nwh = _nwh;

        createSurface();

        VkBool32 presentSupported = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIdx, m_surface, &presentSupported));
        assert(presentSupported);

        createSwapchain();

        return VK_SUCCESS;
    }

    void Swapchain_vk::createSwapchain()
    {
        const VkDevice device = s_renderVK->m_device;
        const VkPhysicalDevice physicalDevice = s_renderVK->m_physicalDevice;

        VkSurfaceCapabilitiesKHR surfaceCaps;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &surfaceCaps));

        uint32_t width = surfaceCaps.currentExtent.width;
        uint32_t height = surfaceCaps.currentExtent.height;

        assert(width == m_resolution.width || height == m_resolution.height);

        VkFormat format = getSwapchainFormat();

        VkCompositeAlphaFlagBitsKHR surfaceComposite =
            (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
            ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
            : (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
            ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
            : (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
            ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
            : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

        VkSurfaceTransformFlagsKHR preTransform = (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
            : surfaceCaps.currentTransform;

        VkPresentModeKHR presentMode = getSwapchainPresentMode(physicalDevice, m_surface);

        m_sci.surface = m_surface;
        m_sci.minImageCount = glm::max(4u, surfaceCaps.minImageCount);
        m_sci.imageFormat = format;
        m_sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        m_sci.imageExtent.width = width;
        m_sci.imageExtent.height = height;
        m_sci.imageArrayLayers = 1;
        m_sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        m_sci.queueFamilyIndexCount = 1;
        m_sci.pQueueFamilyIndices = NULL;
        m_sci.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
        m_sci.compositeAlpha = surfaceComposite;
        m_sci.presentMode = presentMode;
        m_sci.clipped = VK_TRUE;

        // Enable transfer source on swap chain images if supported
        if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            m_sci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        // Enable transfer destination on swap chain images if supported
        if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            m_sci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        VK_CHECK(vkCreateSwapchainKHR(device, &m_sci, 0, &m_swapchain));


        m_sci.oldSwapchain = m_swapchain;

        // count only
        VK_CHECK(vkGetSwapchainImagesKHR(device, m_swapchain, &m_swapchainImageCount, NULL));
        if (m_swapchainImageCount < m_sci.minImageCount)
        {
            message(error, "create swapchain failed! %d < %d", m_swapchainImageCount, m_sci.minImageCount);
        }

        if (m_swapchainImageCount > COUNTOF(m_swapchainImages))
        {
            message(error, "create swapchain failed! %d > %d", m_swapchainImageCount, COUNTOF(m_swapchainImages));
        }

        VK_CHECK(vkGetSwapchainImagesKHR(device, m_swapchain, &m_swapchainImageCount, &m_swapchainImages[0]));
    }

    void Swapchain_vk::createSurface()
    {
        const VkDevice device = s_renderVK->m_device;
        const VkInstance instance = s_renderVK->m_instance;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        info.hinstance = (HINSTANCE)GetModuleHandle(0);
        info.hwnd = (HWND)m_nwh;
        VkSurfaceKHR surface;
        VK_CHECK(vkCreateWin32SurfaceKHR(instance, &info, 0, &surface));
        
        m_surface = surface;
#else
#error unsupport platform
#endif
    }

    VkFormat Swapchain_vk::getSwapchainFormat()
    {
        const VkPhysicalDevice physicalDevice = s_renderVK->m_physicalDevice;

        stl::vector<VkSurfaceFormatKHR> formats;
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, nullptr);
        formats.resize(formatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, formats.data()));

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

    kage::vk::SwapchainStatus_vk Swapchain_vk::getSwapchainStatus()
    {
        const VkPhysicalDevice physicalDevice = s_renderVK->m_physicalDevice;

        VkSurfaceCapabilitiesKHR surfaceCaps;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &surfaceCaps));

        if (m_resolution.width == surfaceCaps.currentExtent.width 
            && m_resolution.height == surfaceCaps.currentExtent.height)
        {
            return SwapchainStatus_vk::ready;
        }

        if (surfaceCaps.currentExtent.width == 0 || surfaceCaps.currentExtent.height == 0)
        {
            return SwapchainStatus_vk::not_ready;
        }

        // destroy old swapchain
        releaseSwapchain();

        // create new
        createSwapchain();

        return SwapchainStatus_vk::resize;
    }

    void Swapchain_vk::releaseSwapchain()
    {
        release(m_swapchain);
    }

    void Swapchain_vk::releaseSurface()
    {
        release(m_surface);
    }

    void Swapchain_vk::destroy()
    {
        releaseSwapchain();
        releaseSurface();
    }

} // namespace vk
} // namespace kage