#include "context.hpp"

#include <stdexcept>
#include <iostream>
#include <set>

using namespace niji;

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                          VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

void Context::init()
{
    init_window();

    create_instance();
    setup_debug_messenger();
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_command_pool();
}

void Context::init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "niji", nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);
}

void Context::cleanup()
{
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

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
    appInfo.apiVersion = VK_API_VERSION_1_0;

    auto extensions = get_required_extensions();
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

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

    // Load Functions for Dynamic Rendering
    BeginRendering =
        (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(m_instance, "vkCmdBeginRenderingKHR");
    EndRendering =
        (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(m_instance, "vkCmdEndRenderingKHR");

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

    if (ENABLE_VALIDATION_LAYERS)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
Context::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                        const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
{
    std::cerr << "validation Layer: " << callbackData->pMessage << std::endl;

    return VK_FALSE;
}

void Context::setup_debug_messenger()
{
    if (!ENABLE_VALIDATION_LAYERS)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    populate_debug_messenger_create_info(createInfo);

    if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to Set Up Debug Messenger!");
    }
}

void Context::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
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

    for (const auto& device : devices)
    {
        if (is_device_suitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("Failed To Find a Suitable GPU!");
}

bool Context::is_device_suitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = QueueFamilyIndices::find_queue_families(device, m_surface);

    bool extensionsSupported = check_device_extension_support(device);
    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport =
            SwapChainSupportDetails::query_swap_chain_support(device, m_surface);
        swapChainAdequate =
            !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures = {};
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.is_complete() && extensionsSupported && swapChainAdequate &&
           supportedFeatures.samplerAnisotropy;
}

void Context::create_logical_device()
{
    QueueFamilyIndices indices =
        QueueFamilyIndices::find_queue_families(m_physicalDevice, m_surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.GraphicsFamily.value(),
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

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature = {};
    dynamicRenderingFeature.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &dynamicRenderingFeature;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
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
    vkGetDeviceQueue(m_device, indices.PresentFamily.value(), 0, &m_presentQueue);
}

bool Context::check_device_extension_support(VkPhysicalDevice device)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());

    std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

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
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                             details.Formats.data());
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

void Context::create_command_pool()
{
    QueueFamilyIndices queueFamilyIndices =
        QueueFamilyIndices::find_queue_families(m_physicalDevice, m_surface);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Command Pool!");
}

uint32_t Context::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties = {};
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("Failed to Find Suitable Memory Type!");
}

VkFormat Context::find_supported_format(const std::vector<VkFormat>& candidates,
                                        VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props = {};
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                 (props.optimalTilingFeatures & features) == features)
            return format;
    }

    throw std::runtime_error("Failed to Find Supported Format!");
}

VkFormat Context::find_depth_format()
{
    return find_supported_format(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool Context::has_stencil_component(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

QueueFamilyIndices QueueFamilyIndices::find_queue_families(VkPhysicalDevice device,
                                                           VkSurfaceKHR surface)
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

        if (indices.is_complete())
            break;

        if (presentSupport)
            indices.PresentFamily = i;

        i++;
    }

    return indices;
}
