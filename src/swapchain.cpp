#include "common.h"
#include "swapchain.h"

#include "resources.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>



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
    createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    createInfo.oldSwapchain = oldSwapchain;

    VkSwapchainKHR swapchain = 0;
    VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain));

    return swapchain;
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


void createSwapchain(Swapchain& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t* familyIndex
    , VkFormat format, VkRenderPass renderPass, VkSwapchainKHR oldSwapchain/* = 0*/)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    uint32_t width = surfaceCaps.currentExtent.width;
    uint32_t height = surfaceCaps.currentExtent.height;

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
