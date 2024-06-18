#pragma once

#include "config.h"
#include "common.h"

#include "kage_structs.h"
#include "kage_rhi_vk.h"

namespace kage { namespace vk
{

    enum class SwapchainStatus_vk : uint32_t
    {
        ready = 0u,
        not_ready = 1u,
        resize = 2u,
    };



    struct Swapchain_vk
    {
        Swapchain_vk()
            : m_swapchain{ VK_NULL_HANDLE }
            , m_surface{ VK_NULL_HANDLE }
            , m_sci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR }
            , m_nwh{ nullptr }
            , m_swapchainImages{}
            , m_swapchainImageCount{ 0 }
            , m_shouldRecreateSwapchain{true}
            , m_shouldPresent{false}
        {
        }

        VkResult create(void* _nwh, const Resolution& _resolution);
        void update(void* _nwh, const Resolution& _resolution);
        void destroy();

        void createSwapchain();
        void createSurface();

        bool acquire(VkCommandBuffer _cmdBuf);
        void present();

        VkFormat getSwapchainFormat();
        SwapchainStatus_vk getSwapchainStatus();

        void releaseSwapchain();
        void releaseSurface();

        void* m_nwh;
        VkSwapchainKHR m_swapchain;
        VkSurfaceKHR m_surface;
        VkSwapchainCreateInfoKHR m_sci;

        VkImage m_swapchainImages[kMaxNumOfBackBuffers];
        uint32_t m_swapchainImageCount;

        uint32_t m_swapchainImageIndex;

        Resolution m_resolution;

        VkSemaphore m_waitSemaphore;
        VkSemaphore m_signalSemaphore;

        bool m_shouldRecreateSwapchain;
        bool m_shouldPresent;
    };

} // namespace vk
} // namespace kage