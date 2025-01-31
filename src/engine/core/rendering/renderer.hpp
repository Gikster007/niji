#pragma once

#include <memory>
#include <string>
#include <array>

#include <glm/glm.hpp>

#include "core/context.hpp"
#include "core/ecs.hpp"

namespace niji
{
struct Vertex
{
    glm::vec3 Pos = {};
    glm::vec3 Color = {};
    glm::vec2 TexCoord = {};

    static VkVertexInputBindingDescription get_binding_description();
    static std::array<VkVertexInputAttributeDescription, 3> get_attribute_description();
};

struct UniformBufferObject
{
    alignas(16) glm::mat4 Model = {};
    alignas(16) glm::mat4 View = {};
    alignas(16) glm::mat4 Proj = {};
};

class Renderer : public System
{
  public:
    Renderer() = default;
    ~Renderer() override;
    Renderer(Context& context);

    void init();

    void update(const float dt) override;
    void render() override;

    void cleanup() override;

  private:
    VkSurfaceFormatKHR choose_swap_surface_format(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR choose_swap_present_mode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);
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
    void record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void create_sync_objects();

    void create_vertex_buffer();
    void create_index_buffer();
    void create_uniform_buffers();
    void update_uniform_buffer(uint32_t currentImage);

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags properties, VkBuffer& buffer,
                       VkDeviceMemory& bufferMemory);
    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer commandBuffer);

    void create_texture_iamge();
    void create_texture_image_view();
    void create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                      VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                      VkDeviceMemory& imageMemory);
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                 VkImageLayout newLayout);
    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void create_texture_sampler();

    void create_depth_resources();

  private:
    std::vector<Vertex> m_vertices = {};
    std::vector<uint16_t> m_indices = {};

    std::vector<VkBuffer> m_uniformBuffers = {};
    std::vector<VkDeviceMemory> m_uniformBuffersMemory = {};
    std::vector<void*> m_uniformBuffersMapped = {};

    VkImage m_textureImage = {};
    VkImageView m_textureImageView = {};
    VkDeviceMemory m_textureImageMemory = {};

    VkImage m_depthImage = {};
    VkImageView m_depthImageView = {};
    VkDeviceMemory m_depthImageMemory = {};

    VkSampler m_textureSampler = {};

    VkBuffer m_vertexBuffer = {};
    VkDeviceMemory m_vertexBufferMemory = {};
    VkBuffer m_indexBuffer = {};
    VkDeviceMemory m_indexBufferMemory = {};

    Context* m_context = nullptr;

    uint32_t m_currentFrame = 0;

    VkSwapchainKHR m_swapChain = {};
    std::vector<VkImage> m_swapChainImages = {};
    VkFormat m_swapChainImageFormat = {};
    VkExtent2D m_swapChainExtent = {};
    std::vector<VkImageView> m_swapChainImageViews = {};

    VkDescriptorPool m_descriptorPool = {};
    std::vector<VkDescriptorSet> m_descriptorSets = {};
    VkDescriptorSetLayout m_descriptorSetLayout = {};
    VkPipelineLayout m_pipelineLayout = {};
    VkPipeline m_graphicsPipeline = {};

    std::vector<VkCommandBuffer> m_commandBuffers =
        {}; // Automatically freed when command pool is destroyed
    std::vector<VkSemaphore> m_imageAvailableSemaphores = {};
    std::vector<VkSemaphore> m_renderFinishedSemaphores = {};
    std::vector<VkFence> m_inFlightFences = {};
};
} // namespace niji
