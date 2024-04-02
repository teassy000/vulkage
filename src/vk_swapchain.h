#pragma once

#include "common.h"


using GLFWwindow = struct GLFWwindow;

namespace kage
{

    struct Swapchain_vk
    {
        VkSwapchainKHR swapchain;

        stl::vector<VkImage> images;

        uint32_t width, height;
        uint32_t imageCount;
    };

    enum class SwapchainStatus_vk : uint32_t
    {
        ready = 0u,
        not_ready = 1u,
        resize = 2u,
    };

    VkSurfaceKHR createSurface(VkInstance instance, void* _wnd);
    VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    void createSwapchain(Swapchain_vk& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t familyIndex, VkFormat format, VkSwapchainKHR oldSwapchain = 0);

    void destroySwapchain(VkDevice device, const Swapchain_vk& swapchain);

    SwapchainStatus_vk resizeSwapchainIfNecessary(Swapchain_vk& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t familyIndex, VkFormat format);

    VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView colorView, VkImageView depthView, uint32_t width, uint32_t height);

} // namespace vkz