#include "precomp.hpp"

#include "app.hpp"

#include <stdexcept>
#include <vector>
#include <iostream>

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
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

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

void App::run()
{
    init_window();
    init_vulkan();
    main_loop();
    cleanup();
}

void App::init_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Molten", nullptr, nullptr);
}

void App::init_vulkan()
{
    create_instance();
    setup_debug_messenger();
}

void App::create_instance()
{
    printf("Creating Vulkan Instance - IN PROGRESS \n");

    if (ENABLE_VALIDATION_LAYERS && !check_validation_layer_support())
        throw std::runtime_error("Validation Layers Requested, But Not Available!");

    // Create Vulkan Instance
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Molten";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Molten Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    auto extensions = get_required_extensions();
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    if (ENABLE_VALIDATION_LAYERS)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();

        populate_debug_messenger_create_info(debug_create_info);
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
    }
    else
    {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
    }

    if (vkCreateInstance(&create_info, nullptr, &m_instance) != VK_SUCCESS)
        throw std::runtime_error("failed to create instance!");

    // Check for Extensions
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions_list(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions_list.data());

    std::cout << "Available extensions:\n";
    for (const auto& extension : extensions_list)
    {
        std::cout << '\t' << extension.extensionName << '\n';
    }

    printf("\n Creating Vulkan Instance - DONE \n");
}

void App::main_loop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
    }
}

void App::cleanup()
{
    if (ENABLE_VALIDATION_LAYERS)
        DestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);

    vkDestroyInstance(m_instance, nullptr);

    glfwDestroyWindow(m_window);

    glfwTerminate();
}

bool App::check_validation_layer_support()
{
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const char* layer_name : VALIDATION_LAYERS)
    {
        bool layer_found = false;

        for (const auto& layer_properties : available_layers)
        {
            if (strcmp(layer_name, layer_properties.layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
        {
            return false;
        }
    }

    return true;
}

std::vector<const char*> App::get_required_extensions()
{
    uint32_t glfw_ext_count = 0;
    const char** glfw_ext;
    glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    std::vector<const char*> extensions(glfw_ext, glfw_ext + glfw_ext_count);

    if (ENABLE_VALIDATION_LAYERS)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
App::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                    VkDebugUtilsMessageTypeFlagsEXT message_type,
                    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{

    std::cerr << "validation layer: " << callback_data->pMessage << std::endl;

    return VK_FALSE;
}

void App::setup_debug_messenger()
{
    if (!ENABLE_VALIDATION_LAYERS)
        return;

    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    populate_debug_messenger_create_info(create_info);

    if (CreateDebugUtilsMessengerEXT(m_instance, &create_info, nullptr, &m_debug_messenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to Set Up Debug Messenger!");
    }
}

void App::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
    create_info = {};
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
}
