#pragma once

#include <memory>
#include <string>
#include <array>

#include <glm/glm.hpp>

#include "core/context.hpp"
#include "core/ecs.hpp"

#include "../core/commandlist.hpp"

enum VmaMemoryUsage;

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

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

class Renderer : System
{
  public:
    Renderer();
    ~Renderer();

    void init();

    void update(const float dt);
    void render();

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

    void create_graphics_pipeline();
    static std::vector<char> read_file(const std::string& filename);
    VkShaderModule create_shader_module(const std::vector<char>& code);

    void create_command_buffers();
    void create_sync_objects();

    void create_uniform_buffers();
    void update_uniform_buffer(uint32_t currentImage);

    void create_depth_resources();

  private:
    friend class Material;

    std::vector<VkBuffer> m_uniformBuffers = {};
    std::vector<VmaAllocation> m_uniformBuffersAllocations = {};
    std::vector<void*> m_uniformBuffersMapped = {};

    VkImage m_depthImage = {};
    VkImageView m_depthImageView = {};
    VmaAllocation m_depthImageAllocation = {};

    Context* m_context = nullptr;

    uint32_t m_currentFrame = 0;

    VkSwapchainKHR m_swapChain = {};
    std::vector<VkImage> m_swapChainImages = {};
    VkFormat m_swapChainImageFormat = {};
    VkExtent2D m_swapChainExtent = {};
    std::vector<VkImageView> m_swapChainImageViews = {};

    VkPipelineLayout m_pipelineLayout = {};
    VkPipeline m_graphicsPipeline = {};

    std::vector<CommandList> m_commandBuffers = {};
    std::vector<VkSemaphore> m_imageAvailableSemaphores = {};
    std::vector<VkSemaphore> m_renderFinishedSemaphores = {};
    std::vector<VkFence> m_inFlightFences = {};
};
} // namespace niji
