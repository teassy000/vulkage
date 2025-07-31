#pragma once

#include "core/config.h"
#include "core/common.h"
#include "core/kage_structs.h"

#include "kage_rhi_vk.h"

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
            , m_swapchainImageCount{ 0 }
            , m_shouldRecreateSwapchain{true}
            , m_shouldPresent{false}
            , m_prevAcquiredSemaphore{ VK_NULL_HANDLE }
            , m_prevRenderedSemaphore{ VK_NULL_HANDLE }
            , m_currentSemaphore{ 0 }
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

        VkSemaphore m_presentDoneSemaphore[kMaxNumOfBackBuffers];
        VkSemaphore m_renderDoneSemaphore[kMaxNumOfBackBuffers];
        uint32_t    m_currentSemaphore;

        VkSemaphore m_prevAcquiredSemaphore;
        VkSemaphore m_prevRenderedSemaphore;

        bool m_shouldRecreateSwapchain;
        bool m_shouldPresent;
    };

} // namespace vk
} // namespace kage