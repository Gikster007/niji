#include "swapchain.hpp"

#include <stdexcept>

#include <vk_mem_alloc.h>

#include "engine.hpp"

using namespace niji;

SwapChainSupportDetails SwapChainSupportDetails::query_swap_chain_support(VkPhysicalDevice device,
                                                                          VkSurfaceKHR surface)
{
    SwapChainSupportDetails details = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                                  details.PresentModes.data());
    }

    return details;
}

Swapchain::Swapchain()
{
    create();

    create_image_views();

    create_depth_resources();
}

void Swapchain::recreate()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(nijiEngine.m_context.m_window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(nijiEngine.m_context.m_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(nijiEngine.m_context.m_device);

    cleanup();

    create();
    create_image_views();
    create_depth_resources();
}

void Swapchain::create()
{
    SwapChainSupportDetails swapChainSupport =
        SwapChainSupportDetails::query_swap_chain_support(nijiEngine.m_context.m_physicalDevice,
                                                          nijiEngine.m_context.m_surface);

    VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.Formats);
    VkPresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.PresentModes);
    VkExtent2D extent = choose_swap_extent(swapChainSupport.Capabilities);

    uint32_t imageCount = swapChainSupport.Capabilities.minImageCount;
    if (swapChainSupport.Capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.Capabilities.maxImageCount)
        imageCount = swapChainSupport.Capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = nijiEngine.m_context.m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices =
        QueueFamilyIndices::find_queue_families(nijiEngine.m_context.m_physicalDevice,
                                                nijiEngine.m_context.m_surface);
    uint32_t queueFamilyIndices[] = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

    if (indices.GraphicsFamily != indices.PresentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(nijiEngine.m_context.m_device, &createInfo, nullptr, &m_object) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Create Swap Chain!");

    vkGetSwapchainImagesKHR(nijiEngine.m_context.m_device, m_object, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(nijiEngine.m_context.m_device, m_object, &imageCount,
                            m_images.data());

    m_format = surfaceFormat.format;
    m_extent = extent;
}

void Swapchain::create_image_views()
{
    m_imageViews.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++)
    {
        m_imageViews[i] = nijiEngine.m_context.create_image_view(m_images[i], m_format,
                                                                 VK_IMAGE_ASPECT_COLOR_BIT, 1, 1);
    }
}

void Swapchain::create_depth_resources()
{
    VkFormat depthFormat = nijiEngine.m_context.find_depth_format();
    nijiEngine.m_context.create_image(m_extent.width, m_extent.height, 1, 1, depthFormat,
                            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                            VMA_MEMORY_USAGE_GPU_ONLY, 0, m_depthImage, m_depthImageAllocation);
    m_depthImageView =
        nijiEngine.m_context.create_image_view(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, 1);

    nijiEngine.m_context.transition_image_layout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
}

void Swapchain::cleanup()
{
    vkDestroyImageView(nijiEngine.m_context.m_device, m_depthImageView, nullptr);
    vmaDestroyImage(nijiEngine.m_context.m_allocator, m_depthImage, m_depthImageAllocation);

    for (auto imageView : m_imageViews)
    {
        vkDestroyImageView(nijiEngine.m_context.m_device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(nijiEngine.m_context.m_device, m_object, nullptr);
}

VkSurfaceFormatKHR Swapchain::choose_swap_surface_format(
    const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }
    return availableFormats[0];
}

VkPresentModeKHR Swapchain::choose_swap_present_mode(
    const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            return availablePresentMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width = -1, height = -1;
        glfwGetFramebufferSize(nijiEngine.m_context.m_window, &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);
        return actualExtent;
    }
}