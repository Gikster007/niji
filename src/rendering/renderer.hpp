#pragma once

#include <memory>
#include <string>
#include <array>

#include <glm/glm.hpp>

#include "core/context.hpp"

namespace molten
{
struct Vertex
{
    glm::vec2 pos = {};
    glm::vec3 color = {};
    
    static VkVertexInputBindingDescription get_binding_description();
    static std::array<VkVertexInputAttributeDescription, 2> get_attribute_description();
};

class Renderer
{
  public:
    Renderer() = default;
    Renderer(Context& context);

    void init();

    void draw();

    void cleanup();

  private:
    VkSurfaceFormatKHR choose_swap_surface_format(
        const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR choose_swap_present_mode(
        const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);
    void create_swap_chain();
    void recreate_swap_chain();
    void create_image_views();
    void cleanup_swap_chain();

    void create_graphics_pipeline();
    static std::vector<char> read_file(const std::string& filename);
    VkShaderModule create_shader_module(const std::vector<char>& code);

    void create_command_buffers();
    void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);
    void create_sync_objects();

    void create_vertex_buffer();
    void create_index_buffer();

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags properties, VkBuffer& buffer,
                       VkDeviceMemory& buffer_memory);
    void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

  private:
    std::vector<Vertex> m_vertices = {};
    std::vector<uint16_t> m_indices = {};
    VkBuffer m_vertex_buffer = {};
    VkDeviceMemory m_vertex_buffer_memory = {};
    VkBuffer m_index_buffer = {};
    VkDeviceMemory m_index_buffer_memory = {};

    Context* m_context = nullptr;

    uint32_t current_frame = 0;

    VkSwapchainKHR m_swap_chain = {};
    std::vector<VkImage> m_swap_chain_images = {};
    VkFormat m_swap_chain_image_format = {};
    VkExtent2D m_swap_chain_extent = {};
    std::vector<VkImageView> m_swap_chain_image_views = {};

    VkPipelineLayout m_pipeline_layout = {};
    VkPipeline m_graphics_pipeline = {};

    std::vector<VkCommandBuffer> m_command_buffers =
        {}; // Automatically freed when command pool is destroyed
    std::vector<VkSemaphore> m_image_available_semaphores = {};
    std::vector<VkSemaphore> m_render_finished_semaphores = {};
    std::vector<VkFence> m_in_flight_fences = {};
};
} // namespace molten
