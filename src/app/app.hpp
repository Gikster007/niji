#pragma once

#include "precomp.hpp"
#include <optional>

class GLFWwindow;

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics_family = {};
    std::optional<uint32_t> present_family = {};

    bool is_complete() const
    {
        return graphics_family.has_value() && present_family.has_value();
    }
};

class App
{
  public:
    void run();

  private:
    void init_window();
    void init_vulkan();

    void create_instance();

    void main_loop();

    void cleanup();

    bool check_validation_layer_support();
    std::vector<const char*> get_required_extensions();
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                   VkDebugUtilsMessageTypeFlagsEXT message_type,
                   const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
    void setup_debug_messenger();
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);

    void pick_physical_device();
    bool is_device_suitable(VkPhysicalDevice device);
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    void create_logical_device();

    void create_surface();
    bool check_device_extension_support(VkPhysicalDevice device);

  public:
  private:
    GLFWwindow* m_window = nullptr;
    VkInstance m_instance = {};
    VkDebugUtilsMessengerEXT m_debug_messenger = {};

    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkDevice m_device = {};
    VkQueue m_graphics_queue = {}; // Doesn't need cleanup. Is implicitly cleaned when device is destroyed
    
    VkSurfaceKHR m_surface = {};
    VkQueue m_present_queue = {};
};