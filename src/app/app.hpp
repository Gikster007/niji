#pragma once

#include "precomp.hpp"

class GLFWwindow;

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

  public:
  private:
    GLFWwindow* m_window = nullptr;
    VkInstance m_instance = {};
    VkDebugUtilsMessengerEXT m_debug_messenger = {};
};