#pragma once

#include <memory>
#include <string>
#include <array>

#include <glm/glm.hpp>

#include "core/descriptor.hpp"
#include "core/context.hpp"
#include "core/ecs.hpp"

#include "passes/render_pass.hpp"
#include "passes/forward_pass.hpp"
#include "passes/imgui_pass.hpp"

#include "swapchain.hpp"

namespace niji
{
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
    void create_sync_objects();
    
    void update_uniform_buffer(uint32_t currentImage);

  private:
    friend class Material;
    friend class ForwardPass;
    friend class ImGuiPass;

    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> m_ubos = {};
    std::vector<CommandList> m_commandBuffers = {};
    std::vector<std::unique_ptr<RenderPass>> m_renderPasses;

    Swapchain m_swapchain = {};

    Context* m_context = nullptr;

    NijiTexture m_fallbackTexture = {};

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex = UINT64_MAX;

    Descriptor m_globalDescriptor = {};
    /*VkDescriptorSetLayout m_globalSetLayout = {};
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_globalDescriptorSet = {};
    VkDescriptorPool m_descriptorPool = {};*/

    std::vector<VkSemaphore> m_imageAvailableSemaphores = {};
    std::vector<VkSemaphore> m_renderFinishedSemaphores = {};
    std::vector<VkFence> m_inFlightFences = {};

};
} // namespace niji
