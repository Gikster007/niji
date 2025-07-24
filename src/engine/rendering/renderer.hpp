#pragma once

#include <memory>
#include <string>
#include <array>

#include <glm/glm.hpp>

#include "core/descriptor.hpp"
#include "core/context.hpp"
#include "core/envmap.hpp"
#include "core/ecs.hpp"

#include "passes/forward_pass.hpp"
#include "passes/render_pass.hpp"
#include "passes/imgui_pass.hpp"

#include "model/mesh.hpp"

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

    void set_envmap(Envmap& envmap)
    {
        m_envmap = &envmap;
    }

  private:
    void create_sync_objects();

    void update_uniform_buffer(uint32_t currentImage);

  private:
    friend class Material;
    friend class ForwardPass;
    friend class ImGuiPass;
    friend class SkyboxPass;

    std::vector<Buffer> m_ubos = {};
    std::vector<CommandList> m_commandBuffers = {};
    std::vector<std::unique_ptr<RenderPass>> m_renderPasses;

    Swapchain m_swapchain = {};

    Mesh m_cube = {};

    Context* m_context = nullptr;
    Envmap* m_envmap = nullptr;

    Texture m_fallbackTexture = {};

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex = UINT64_MAX;

    Descriptor m_globalDescriptor = {};

    std::vector<VkSemaphore> m_imageAvailableSemaphores = {};
    std::vector<VkSemaphore> m_renderFinishedSemaphores = {};
    std::vector<VkFence> m_inFlightFences = {};
};
} // namespace niji
