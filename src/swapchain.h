#pragma once


struct Swapchain
{
    VkSwapchainKHR swapchain;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;


    uint32_t width, height;
    uint32_t imageCount;
};

typedef struct GLFWwindow GLFWwindow;

VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window);
VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

VkSwapchainKHR createSwapchain(VkDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR surfaceCaps, uint32_t* familyIndex, VkFormat format, VkSwapchainKHR oldSwapchain);
void createSwapchain(Swapchain& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t* familyIndex, VkFormat format, VkRenderPass renderPass, VkSwapchainKHR oldSwapchain = 0);

void destroySwapchain(VkDevice device, const Swapchain& swapchain);

void resizeSwapchainIfNecessary(Swapchain& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t* familyIndex, VkFormat format, VkRenderPass renderPass);