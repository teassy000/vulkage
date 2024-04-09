#pragma once

#include "common.h"


using GLFWwindow = struct GLFWwindow;

namespace kage
{

    struct Swapchain_vk
    {
        Swapchain_vk()
            : m_swapchain{ VK_NULL_HANDLE }
            , m_backBuffers{}
            , m_width{ 0 }
            , m_height{ 0 }
            , m_imgCnt{ 0 }
        {
        }

        VkResult create();

        VkSwapchainKHR m_swapchain;

        stl::vector<VkImage> m_backBuffers;

        uint32_t m_width, m_height;
        uint32_t m_imgCnt;
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

} // namespace kage