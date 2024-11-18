#pragma once

#include <memory>
#include <string>
#include <array>

#include <glm/glm.hpp>

#include "core/context.hpp"

namespace niji
{
struct Vertex
{
    glm::vec3 pos = {};
    glm::vec3 color = {};
    glm::vec2 tex_coord = {};

    static VkVertexInputBindingDescription get_binding_description();
    static std::array<VkVertexInputAttributeDescription, 3> get_attribute_description();
};

struct UniformBufferObject
{
    alignas(16) glm::mat4 model = {};
    alignas(16) glm::mat4 view = {};
    alignas(16) glm::mat4 proj = {};
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

    void create_descriptor_pool();
    void create_descriptor_sets();
    void create_descriptor_set_layout();
    void create_graphics_pipeline();
    static std::vector<char> read_file(const std::string& filename);
    VkShaderModule create_shader_module(const std::vector<char>& code);

    void create_command_buffers();
    void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);
    void create_sync_objects();

    void create_vertex_buffer();
    void create_index_buffer();
    void create_uniform_buffers();
    void update_uniform_buffer(uint32_t current_image);

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags properties, VkBuffer& buffer,
                       VkDeviceMemory& buffer_memory);
    void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
    
    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);

    void create_texture_iamge();
    void create_texture_image_view();
    void create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                      VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                      VkDeviceMemory& image_memory);
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);
    void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void create_texture_sampler();

    void create_depth_resources();

  private:
    std::vector<Vertex> m_vertices = {};
    std::vector<uint16_t> m_indices = {};

    std::vector<VkBuffer> m_uniform_buffers = {};
    std::vector<VkDeviceMemory> m_uniform_buffers_memory = {};
    std::vector<void*> m_uniform_buffers_mapped = {};

    VkImage m_texture_image = {};
    VkImageView m_texture_image_view = {};
    VkDeviceMemory m_texture_image_memory = {};

    VkImage m_depth_image = {};
    VkImageView m_depth_image_view = {};
    VkDeviceMemory m_depth_image_memory = {};

    VkSampler m_texture_sampler = {};

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

    VkDescriptorPool m_descriptor_pool = {};
    std::vector<VkDescriptorSet> m_descriptor_sets = {};
    VkDescriptorSetLayout m_descriptor_set_layout = {};
    VkPipelineLayout m_pipeline_layout = {};
    VkPipeline m_graphics_pipeline = {};

    std::vector<VkCommandBuffer> m_command_buffers =
        {}; // Automatically freed when command pool is destroyed
    std::vector<VkSemaphore> m_image_available_semaphores = {};
    std::vector<VkSemaphore> m_render_finished_semaphores = {};
    std::vector<VkFence> m_in_flight_fences = {};
};
} // namespace niji
