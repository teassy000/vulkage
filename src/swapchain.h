#pragma once


struct Swapchain
{
    VkSwapchainKHR swapchain;

    std::vector<VkImage> images;

    uint32_t width, height;
    uint32_t imageCount;
};

enum SwapchainStatus
{
    Ready,
    NotReady,
    Resized,
};

typedef struct GLFWwindow GLFWwindow;

VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window);
VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

VkSwapchainKHR createSwapchain(VkDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR surfaceCaps, uint32_t* familyIndex, VkFormat format, VkSwapchainKHR oldSwapchain);
void createSwapchain(Swapchain& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t* familyIndex, VkFormat format, VkSwapchainKHR oldSwapchain = 0);

void destroySwapchain(VkDevice device, const Swapchain& swapchain);

SwapchainStatus resizeSwapchainIfNecessary(Swapchain& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t* familyIndex, VkFormat format);

VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView colorView, VkImageView depthView, uint32_t width, uint32_t height);