#pragma once

#include "common.h"
#include "config.h"


using GLFWwindow = struct GLFWwindow;

namespace kage { namespace vk
{

    struct Swapchain_vk
    {
        Swapchain_vk()
            : m_swapchain{ VK_NULL_HANDLE }
            , m_surface{ VK_NULL_HANDLE }
            , m_sci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR }
            , m_nwh{ nullptr }
            , m_swapchainImages{}
            , m_width{ 0 }
            , m_height{ 0 }
            , m_swapchainImageCount{ 0 }
        {
        }

        VkResult create(void* _nwh);

        void createSwapchain();

        void createSurface();

        VkFormat getSwapchainFormat();

        void destroy();

        void* m_nwh;
        VkSwapchainKHR m_swapchain;
        VkSurfaceKHR m_surface;
        VkSwapchainCreateInfoKHR m_sci;

        VkImage m_swapchainImages[kMaxNumOfBackBuffers];
        uint32_t m_swapchainImageCount;
        uint32_t m_width, m_height;

    };

    enum class SwapchainStatus_vk : uint32_t
    {
        ready = 0u,
        not_ready = 1u,
        resize = 2u,
    };

    SwapchainStatus_vk resizeSwapchainIfNecessary(Swapchain_vk& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t familyIndex, VkFormat format);

    VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView colorView, VkImageView depthView, uint32_t width, uint32_t height);
} // namespace vk
} // namespace kage