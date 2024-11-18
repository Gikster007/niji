#pragma once

#include <optional>

class GLFWwindow;

namespace niji
{
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics_family = {};
    std::optional<uint32_t> present_family = {};

    bool is_complete() const
    {
        return graphics_family.has_value() && present_family.has_value();
    }
    static QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
    static SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device,
                                                            VkSurfaceKHR surface);
};

class Context
{
    friend class Renderer;
    friend class Engine;

  public:
    Context() = default;
    void init();

    void init_window();

    void cleanup();

    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

  private:
    void create_instance();
    bool check_validation_layer_support();
    std::vector<const char*> get_required_extensions();
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                   VkDebugUtilsMessageTypeFlagsEXT message_type,
                   const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
    void setup_debug_messenger();
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);

    void create_surface();

    void pick_physical_device();
    bool is_device_suitable(VkPhysicalDevice device);
    void create_logical_device();
    bool check_device_extension_support(VkPhysicalDevice device);

    void create_command_pool();

    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
    VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                   VkFormatFeatureFlags features);
    VkFormat find_depth_format();
    bool has_stencil_component(VkFormat format);

  private:
    PFN_vkCmdBeginRenderingKHR BeginRendering = nullptr;
    PFN_vkCmdEndRenderingKHR EndRendering = nullptr;

    GLFWwindow* m_window = nullptr;
    bool m_framebuffer_resized = false;
    VkInstance m_instance = {};
    VkDebugUtilsMessengerEXT m_debug_messenger = {};

    VkSurfaceKHR m_surface = {};

    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkDevice m_device = {};
    VkQueue m_graphics_queue =
        {}; // Doesn't need cleanup. Is implicitly cleaned when device is destroyed
    VkQueue m_present_queue = {};
    VkCommandPool m_command_pool = {};
};
} // namespace niji
