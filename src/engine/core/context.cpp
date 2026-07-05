#include "context.hpp"

#include <stdexcept>
#include <iostream>
#include <set>

#include <vk_mem_alloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "common.hpp"

#include "rendering/swapchain.hpp"
#include "rendering/resources/texture.hpp"

#include "vulkan-functions.hpp"

using namespace niji;

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

Context::Context()
{
    init_window();

    create_instance();
    setup_debug_messenger();
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_command_pool();

    load_vulkan_function_pointers(m_device);

    // init_allocator();
}

void Context::init()
{
    //// Init Global Sampler
    //{
    //    SamplerDesc desc = {};
    //    desc.MagFilter = SamplerDesc::Filter::LINEAR;
    //    desc.MinFilter = SamplerDesc::Filter::LINEAR;
    //    desc.AddressModeU = SamplerDesc::AddressMode::REPEAT;
    //    desc.AddressModeV = SamplerDesc::AddressMode::REPEAT;
    //    desc.AddressModeW = SamplerDesc::AddressMode::REPEAT;
    //    desc.EnableAnisotropy = true;
    //    desc.MipmapMode = SamplerDesc::MipMapMode::LINEAR;

    //    m_globalSampler = engine;
    //}
}

void Context::init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (mode->height <= 1080 && mode->height <= 1920)
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    m_window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "niji", nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);
}

void Context::deinit()
{
    // m_globalSampler.deinit();

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

#if DEBUG_ALLOCATIONS
    char* statsString = nullptr;
    vmaBuildStatsString(m_allocator, &statsString, VK_TRUE);
    printf("%s\n", statsString);
    vmaFreeStatsString(m_allocator, statsString);
#endif // DEBUG_ALLOCATIONS

    // vmaDestroyAllocator(m_allocator);

    vkDestroyDevice(m_device, nullptr);

    if (ENABLE_VALIDATION_LAYERS)
        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Context::framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
    auto context = reinterpret_cast<Context*>(glfwGetWindowUserPointer(window));
    context->m_framebufferResized = true;
}

void Context::get_window_size(int& width, int& height)
{
    glfwGetWindowSize(m_window, &width, &height);
}

// void Context::init_allocator()
//{
//
// }

void Context::create_instance()
{
    printf("Creating Vulkan Instance - IN PROGRESS \n");

    if (ENABLE_VALIDATION_LAYERS && !check_validation_layer_support())
        throw std::runtime_error("Validation Layers Requested, But Not Available!");

    // Create Vulkan Instance
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "niji";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "niji Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    auto extensions = get_required_extensions();
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

    VkValidationFeatureEnableEXT enables[] = {/*
                                      VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,*/
                                              VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
                                              VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};

    VkValidationFeaturesEXT validationFeatures = {};
    validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationFeatures.enabledValidationFeatureCount = 3;
    validationFeatures.pEnabledValidationFeatures = enables;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    if (ENABLE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

        populate_debug_messenger_create_info(debugCreateInfo);
        // debugCreateInfo.pNext = &validationFeatures;
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
        throw std::runtime_error("failed to create instance!");

    // Check for Extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensionsList(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionsList.data());

    std::cout << "Available extensions:\n";
    for (const auto& extension : extensionsList)
    {
        std::cout << '\t' << extension.extensionName << '\n';
    }

    printf("\n Creating Vulkan Instance - DONE \n");
}

bool Context::check_validation_layer_support()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : VALIDATION_LAYERS)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

std::vector<const char*> Context::get_required_extensions()
{
    uint32_t glfwExtCount = 0;
    const char** glfwExt;
    glfwExt = glfwGetRequiredInstanceExtensions(&glfwExtCount);

    std::vector<const char*> extensions(glfwExt, glfwExt + glfwExtCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Context::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                       VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                       const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
{
    std::cerr << "validation Layer: " << callbackData->pMessage << std::endl;
    std::cerr << "----------------------------" << std::endl;

    return VK_FALSE;
}

void Context::setup_debug_messenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    populate_debug_messenger_create_info(createInfo);

    if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to Set Up Debug Messenger!");
    }
}

void Context::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debug_callback;
}

void Context::create_surface()
{
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
        throw std::runtime_error("Failed To Create Window Surface!");
}

void Context::pick_physical_device()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
    if (device_count == 0)
        throw std::runtime_error("Failed To Find GPUs With Vulkan Support!");

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

    VkPhysicalDevice backup = {};

    VkPhysicalDeviceProperties props = {};
    for (const auto& device : devices)
    {
        vkGetPhysicalDeviceProperties(device, &props);

        if (is_device_suitable(device))
        {
            if (props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                backup = device;
                continue;
            }
            else
            {
                m_physicalDevice = device;
                break;
            }
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE && backup != VK_NULL_HANDLE)
        m_physicalDevice = backup;

    if (m_physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("Failed To Find a Suitable GPU!");

    printf("Picked %s For Rendering \n", props.deviceName);
}

bool Context::is_device_suitable(VkPhysicalDevice device)
{
    m_queueIndices = QueueFamilyIndices::find_queue_families(device, m_surface);

    bool extensionsSupported = check_device_extension_support(device);
    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        VkSurfaceCapabilitiesKHR capabilities {};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

        std::vector<VkSurfaceFormatKHR> formats {};
        if (formatCount != 0)
        {
            formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

        std::vector<VkPresentModeKHR> presentModes {};
        if (presentModeCount != 0)
        {
            presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, presentModes.data());
        }

        swapChainAdequate = !formats.empty() && !presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures = {};
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return m_queueIndices.is_complete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy &&
           supportedFeatures.fillModeNonSolid;
}

void Context::create_logical_device()
{
    QueueFamilyIndices indices = QueueFamilyIndices::find_queue_families(m_physicalDevice, m_surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.GraphicsFamily.value(), indices.TransferFamily.value(),
                                              indices.PresentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Features
    // Vulkan 1.0 Features
    VkPhysicalDeviceFeatures2 features2 {};
    {
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.features.samplerAnisotropy = VK_TRUE;
        features2.features.fillModeNonSolid = VK_TRUE;

        // Vulkan 1.2 Features (BDA + Descriptor Indexing)
        VkPhysicalDeviceVulkan12Features features12 {};
        features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features12.bufferDeviceAddress = VK_TRUE;
        features12.descriptorIndexing = VK_TRUE;
        features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        features12.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
        features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        features12.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
        features12.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
        features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
        features12.descriptorBindingPartiallyBound = VK_TRUE;
        features12.runtimeDescriptorArray = VK_TRUE;

        // Vulkan 1.3 Features (Dynamic Rendering + Synchronization2)
        VkPhysicalDeviceVulkan13Features features13 {};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features13.dynamicRendering = VK_TRUE;
        features13.synchronization2 = VK_TRUE;

        // Add graphics pipeline library features
        VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT graphicsPipelineLib = {};
        graphicsPipelineLib.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT;
        graphicsPipelineLib.graphicsPipelineLibrary = VK_TRUE;

        // Unified Image Layouts
        VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR unifiedLayoutsFeatures {};
        unifiedLayoutsFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR;
        unifiedLayoutsFeatures.unifiedImageLayouts = VK_TRUE;

        // Check for Unified Image Layouts Extension Support
        VkPhysicalDeviceFeatures2 query {};
        query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        query.pNext = &unifiedLayoutsFeatures;

        vkGetPhysicalDeviceFeatures2(m_physicalDevice, &query);

        // Chain
        features2.pNext = &features12;
        features12.pNext = &features13;
        features13.pNext = &graphicsPipelineLib;
        graphicsPipelineLib.pNext = &unifiedLayoutsFeatures;
    }

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features2;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
    if (ENABLE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }
    else
        createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
        throw std::runtime_error("Failed To Create Logical Device!");

    vkGetDeviceQueue(m_device, indices.GraphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.TransferFamily.value(), 0, &m_transferQueue);
    vkGetDeviceQueue(m_device, indices.PresentFamily.value(), 0, &m_presentQueue);
}

bool Context::check_device_extension_support(VkPhysicalDevice device)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void Context::create_command_pool()
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_queueIndices.GraphicsFamily.value();

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Command Pool!");

    SetObjectName(m_device, VK_OBJECT_TYPE_COMMAND_POOL, m_commandPool, "Command Pool");
}

uint32_t Context::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memProperties = {};
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("Failed to Find Suitable Memory Type!");
}

VkFormat Context::find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                        VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props = {};
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    throw std::runtime_error("Failed to Find Supported Format!");
}

VkFormat Context::find_depth_format()
{
    return find_supported_format({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                 VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool Context::has_stencil_component(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkCommandBuffer Context::begin_single_time_commands() const
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer = {};
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void Context::end_single_time_commands(VkCommandBuffer commandBuffer) const
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

static inline bool is_depth_format(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Context::transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                                      uint32_t mipLevels, uint32_t layerCount)
{
    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    // Decide aspect mask
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
        oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && is_depth_format(format))
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (has_stencil_component(format))
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    // Handle known transition types
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (has_stencil_component(format))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (has_stencil_component(format))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    else
    {
        std::cerr << "[WARN] Unhandled layout transition: " << oldLayout << " -> " << newLayout << std::endl;
        end_single_time_commands(commandBuffer);
        return;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    end_single_time_commands(commandBuffer);
}

void Context::copy_buffer_to_image(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height, uint32_t layerCount,
                                   uint32_t baseArrayLayer, uint32_t mipLevel)
{
    VkCommandBuffer cmd = begin_single_time_commands();

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = baseArrayLayer;
    region.imageSubresource.layerCount = layerCount;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    end_single_time_commands(cmd);
}

void Context::generateMipmaps(VkImage image, VkFormat format, uint32_t width, uint32_t height, uint32_t mipLevels)
{
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = width;
    int32_t mipHeight = height;

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        VkImageBlit blit {};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                       &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                             0, nullptr, 1, &barrier);

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    end_single_time_commands(commandBuffer);
}

QueueFamilyIndices QueueFamilyIndices::find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices = {};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        VkBool32 presentSupport = {};
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.GraphicsFamily = i;

        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
            indices.TransferFamily = i;

        if (indices.is_complete())
            break;

        if (presentSupport)
            indices.PresentFamily = i;

        i++;
    }

    return indices;
}
