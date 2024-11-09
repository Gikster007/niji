#pragma once

#include <memory>
#include <string>

#include "core/context.hpp"

namespace molten
{
class Renderer
{
  public:
    Renderer() = default;
    Renderer(std::shared_ptr<Context> context);

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

  private:
    std::shared_ptr<Context> m_context = nullptr;

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
