//#include "swapchain.hpp"
//
//#include <stdexcept>
//#include <algorithm>
//
//#include <vk_mem_alloc.h>
//
//#include "engine.hpp"
//#include "core/context.hpp"
//
//using namespace niji;
//
//SwapChainSupportDetails SwapChainSupportDetails::query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
//{
//    SwapChainSupportDetails details = {};
//    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);
//
//    uint32_t formatCount = 0;
//    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
//
//    if (formatCount != 0)
//    {
//        details.Formats.resize(formatCount);
//        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
//    }
//
//    uint32_t presentModeCount = 0;
//    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
//
//    if (presentModeCount != 0)
//    {
//        details.PresentModes.resize(presentModeCount);
//        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
//    }
//
//    return details;
//}
//
//Swapchain::Swapchain()
//{
//    const SwapChainSupportDetails swapChainSupport =
//        SwapChainSupportDetails::query_swap_chain_support(nijiEngine.m_context.m_physicalDevice, nijiEngine.m_context.m_surface);
//
//    const VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.Formats);
//    m_surfaceFormat = surfaceFormat.format;
//    m_colorSpace = surfaceFormat.colorSpace;
//
//    m_presentMode = choose_swap_present_mode(swapChainSupport.PresentModes);
//    m_extent = choose_swap_extent(swapChainSupport.Capabilities);
//
//    m_imageCount = swapChainSupport.Capabilities.minImageCount;
//    if (swapChainSupport.Capabilities.maxImageCount > 0 && m_imageCount > swapChainSupport.Capabilities.maxImageCount)
//        m_imageCount = swapChainSupport.Capabilities.maxImageCount;
//
//    create();
//    create_image_views();
//}
//
//void Swapchain::recreate()
//{
//    int width = 0, height = 0;
//    glfwGetFramebufferSize(nijiEngine.m_context.m_window, &width, &height);
//    while (width == 0 || height == 0)
//    {
//        glfwGetFramebufferSize(nijiEngine.m_context.m_window, &width, &height);
//        glfwWaitEvents();
//    }
//
//    vkDeviceWaitIdle(nijiEngine.m_context.m_device);
//
//    cleanup();
//
//    create();
//    create_image_views();
//}
//
//void Swapchain::create()
//{
//    
//}
//
//void Swapchain::create_image_views()
//{
//    const VkSemaphoreCreateInfo semaphoreInfo {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
//
//    m_imageViews.resize(m_imageCount);
//    for (size_t i = 0; i < m_imageCount; i++)
//    {
//        // Image View Creation Info
//        VkImageViewCreateInfo viewInfo {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
//        viewInfo.image = m_images[i];
//        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//        viewInfo.format = m_surfaceFormat;
//        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};
//        m_layouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;
//
//        // Create Image View
//        if (vkCreateImageView(nijiEngine.m_context.m_device, &viewInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS)
//        {
//            assert(!"[Swapchain] Failed to Create Image View for the Swapchain Render Target");
//        }
//        const std::string imageViewName = "Swapchain Image View #" + std::to_string(i);
//        SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_IMAGE_VIEW, m_imageViews[i], imageViewName.c_str());
//
//        // Create Image Presentation Semaphore
//        if (vkCreateSemaphore(nijiEngine.m_context.m_device, &semaphoreInfo, nullptr, &m_semaphores[i]) != VK_SUCCESS)
//        {
//            assert(!"[Swapchain] Failed to Create Semaphore for Swapchain Render Target");
//        }
//        const std::string semaphoreName = "Swapchain Image Semaphore #" + std::to_string(i);
//        SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_SEMAPHORE, m_semaphores[i], semaphoreName.c_str());
//    }
//}
//
//void Swapchain::cleanup()
//{
//    m_images.clear();
//    m_layouts.clear();
//    for (uint32_t i = 0u; i < m_imageCount; i++)
//    {
//        vkDestroyImageView(nijiEngine.m_context.m_device, m_imageViews[i], nullptr);
//        vkDestroySemaphore(nijiEngine.m_context.m_device, m_semaphores[i], nullptr);
//    }
//    m_imageViews.clear();
//    m_semaphores.clear();
//
//    vkDestroySwapchainKHR(nijiEngine.m_context.m_device, m_object, nullptr);
//}
//
//VkSurfaceFormatKHR Swapchain::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats)
//{
//    for (const auto& availableFormat : availableFormats)
//    {
//        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
//            return availableFormat;
//    }
//    return availableFormats[0];
//}
//
//VkPresentModeKHR Swapchain::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes)
//{
//    for (const auto& availablePresentMode : availablePresentModes)
//    {
//        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
//            return availablePresentMode;
//    }
//    return VK_PRESENT_MODE_FIFO_KHR;
//}
//
//VkExtent2D Swapchain::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
//{
//    if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
//    {
//        return capabilities.currentExtent;
//    }
//    else
//    {
//        int width = -1, height = -1;
//        glfwGetFramebufferSize(nijiEngine.m_context.m_window, &width, &height);
//
//        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
//
//        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
//        actualExtent.height =
//            std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
//        return actualExtent;
//    }
//}