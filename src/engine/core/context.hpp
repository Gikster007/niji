#pragma once

#include <optional>

#include "rendering/resources/sampler.hpp"

class GLFWwindow;

struct VmaAllocator_T;
typedef VmaAllocator_T* VmaAllocator;

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

typedef VkFlags VmaAllocationCreateFlags;

enum VmaMemoryUsage;

class CameraSystem;

namespace niji
{
struct QueueFamilyIndices
{
    std::optional<uint32_t> GraphicsFamily = {};
    std::optional<uint32_t> TransferFamily = {};
    std::optional<uint32_t> PresentFamily = {};

    bool is_complete() const
    {
        return GraphicsFamily.has_value() && TransferFamily.has_value() && PresentFamily.has_value();
    }
    static QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);
};

class Context
{
  public:
    Context();
    void init();

    void init_window();

    void deinit();

    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

    void get_window_size(int& width, int& height);

  public:
    //void init_allocator();
    void create_instance();
    bool check_validation_layer_support();
    std::vector<const char*> get_required_extensions();
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                   VkDebugUtilsMessageTypeFlagsEXT messageType,
                   const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);
    void setup_debug_messenger();
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void create_surface();

    void pick_physical_device();
    bool is_device_suitable(VkPhysicalDevice device);
    void create_logical_device();
    bool check_device_extension_support(VkPhysicalDevice device);

    void create_command_pool();

    uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                   VkFormatFeatureFlags features) const;
    VkFormat find_depth_format();
    bool has_stencil_component(VkFormat format);

  public:
    VkCommandBuffer begin_single_time_commands() const;

    void end_single_time_commands(VkCommandBuffer commandBuffer) const;

    void transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                 VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount);
    void copy_buffer_to_image(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height,
                              uint32_t layerCount, uint32_t baseArrayLayer, uint32_t mipLevel);
    void generateMipmaps(VkImage image, VkFormat format, uint32_t width, uint32_t height,
                         uint32_t mipLevels);

  public:
    GLFWwindow* m_window = nullptr;
    bool m_framebufferResized = false;
    VkInstance m_instance = {};
    VkDebugUtilsMessengerEXT m_debugMessenger = {};

    VkSurfaceKHR m_surface = {};

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = {};
    QueueFamilyIndices m_queueIndices = {};
    VkQueue m_graphicsQueue = {};
    VkQueue m_transferQueue = {};
    VkQueue m_presentQueue = {};
    VkCommandPool m_commandPool = {};
};
} // namespace niji
