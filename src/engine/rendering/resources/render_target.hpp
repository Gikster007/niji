#pragma once

namespace niji
{

struct RenderTarget
{
    // Surface Resources
    uint32_t ImageCount = 0u;
    VkFormat SurfaceFormat = {};
    VkColorSpaceKHR ColorSpace = {};
    VkPresentModeKHR PresentMode = {};

    // Swapchain Resources
    VkSwapchainKHR Handle = {};
    VkExtent2D Extent = {};
    uint32_t CurrentImage = 0u;

    std::vector<VkImage> Images = {};
    std::vector<VkImageView> ImageViews = {};
    std::vector<VkSemaphore> Semaphores = {};
    std::vector<VkImageLayout> Layouts = {};
};

}