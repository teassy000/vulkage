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

    VkResult Swapchain_vk::create(void* _nwh, const Resolution& _resolution)
    {
        const VkDevice device = s_renderVK->m_device;
        const VkPhysicalDevice physicalDevice = s_renderVK->m_physicalDevice;
        const uint32_t queueFamilyIdx = s_renderVK->m_gfxFamilyIdx;
        
        m_nwh = _nwh;
        m_resolution = _resolution;

        createSurface();

        VkBool32 presentSupported = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIdx, m_surface, &presentSupported));
        assert(presentSupported);

        createSwapchain();


        return VK_SUCCESS;
    }

    void Swapchain_vk::update(void* _nwh, const Resolution& _resolution)
    {
        const bool recreateSurface = false
            || m_nwh != _nwh
            ;

        const bool recreateSwapchain = false
            || m_resolution.format != _resolution.format
            || m_resolution.width != _resolution.width
            || m_resolution.height != _resolution.height
            || recreateSurface
            ;

        m_nwh = _nwh;
        m_resolution = _resolution;

        if (recreateSwapchain)
        {
            releaseSwapchain();

            if (recreateSurface)
            {
                releaseSurface();
                s_renderVK->kick(true);
                m_sci.oldSwapchain = VK_NULL_HANDLE;
                createSurface();
            }

            // wait for the next frame to recreate swapchain
            s_renderVK->kick(true);
            m_sci.oldSwapchain = VK_NULL_HANDLE;
            
            createSwapchain();
        }
    }

    void Swapchain_vk::createSwapchain()
    {
        const VkDevice device = s_renderVK->m_device;
        const VkPhysicalDevice physicalDevice = s_renderVK->m_physicalDevice;

        VkSurfaceCapabilitiesKHR surfaceCaps;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &surfaceCaps));

        const uint32_t width = bx::clamp<uint32_t>(
            m_resolution.width
            , surfaceCaps.minImageExtent.width
            , surfaceCaps.maxImageExtent.width
        );
        const uint32_t height = bx::clamp<uint32_t>(
            m_resolution.height
            , surfaceCaps.minImageExtent.height
            , surfaceCaps.maxImageExtent.height
        );

        BX_ASSERT( 
            (width == m_resolution.width && height == m_resolution.height)
            , "extent should match with the surfce"
        );

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

        {
            VkSemaphoreCreateInfo ci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            VK_CHECK(vkCreateSemaphore(device, &ci, 0, &m_waitSemaphore));
        }

        {
            VkSemaphoreCreateInfo ci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            VK_CHECK(vkCreateSemaphore(device, &ci, 0, &m_signalSemaphore));
        }
        m_shouldPresent = false;
        m_shouldRecreateSwapchain = false;
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

    bool Swapchain_vk::acquire(VkCommandBuffer _cmdBuf)
    {
        if (   VK_NULL_HANDLE == m_swapchain
            || m_shouldRecreateSwapchain
            )
        {
            return false;
        }

        if (!m_shouldPresent)
        {
            const VkDevice device = s_renderVK->m_device;

            VK_CHECK(vkAcquireNextImageKHR(
                device
                , m_swapchain
                , ~0ull
                , m_waitSemaphore
                , VK_NULL_HANDLE
                , &m_swapchainImageIndex
            ));

            m_shouldPresent = true;

            //TODO: fence here?

        }
        return true;
    }

    void Swapchain_vk::present()
    {
        if (VK_NULL_HANDLE ==  m_swapchain
            && m_shouldPresent
            )
        {
            return;
        }

        const VkQueue queue = s_renderVK->m_queue;

        KG_ZoneScopedNC("present", Color::light_yellow);
        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapchain;
        presentInfo.pImageIndices = &m_swapchainImageIndex;
        presentInfo.pWaitSemaphores = &m_signalSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        VkResult result = vkQueuePresentKHR(queue, &presentInfo);

        switch (result)
        {
        case VK_ERROR_OUT_OF_DATE_KHR:
            m_shouldRecreateSwapchain = true;
            break;
        default:
            BX_ASSERT(VK_SUCCESS == result, "vkQueuePresentKHR(...); VK error 0x%x", result);
            break;
        }

        m_shouldPresent = false;
        //m_signalSemaphore = VK_NULL_HANDLE;
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

        m_nwh = nullptr;
    }

} // namespace vk
} // namespace kage