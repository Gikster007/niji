#pragma once

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

namespace niji
{
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR Capabilities = {};
    std::vector<VkSurfaceFormatKHR> Formats = {};
    std::vector<VkPresentModeKHR> PresentModes = {};

    static SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device,
                                                            VkSurfaceKHR surface);
};

class Swapchain
{
  public:
    Swapchain();

    void recreate();

  private:
    void create();
    void create_image_views();
    void create_depth_resources();

    void cleanup();

    VkSurfaceFormatKHR choose_swap_surface_format(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR choose_swap_present_mode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

  private:
    friend class Renderer;
    friend class ForwardPass;

    VkImage m_depthImage = {};
    VkImageView m_depthImageView = {};
    VmaAllocation m_depthImageAllocation = {};

    VkSwapchainKHR m_object = {};
    std::vector<VkImage> m_images = {};
    std::vector<VkImageView> m_imageViews = {};
    VkFormat m_format = {};
    VkExtent2D m_extent = {};
};
} // namespace niji
