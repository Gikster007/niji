#pragma once

#include <memory>
#include <string>
#include <array>

#include <glm/glm.hpp>

#include "core/descriptor.hpp"
#include "core/context.hpp"
#include "core/envmap.hpp"
#include "core/ecs.hpp"

#include "model/mesh.hpp"

#include "swapchain.hpp"


namespace niji
{

struct Sphere
{
    glm::vec3 Center = {0.0f, 0.0f, 0.0f};
    float Radius = 0.0f;
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

    void set_envmap(Envmap& envmap)
    {
        m_envmap = &envmap;
    }

  private:
    void create_sync_objects();

    void update_uniform_buffer(uint32_t currentImage);

  private:
    friend class Material;
    friend class RenderPass;
    friend class ForwardPass;
    friend class ImGuiPass;
    friend class SkyboxPass;
    friend class LineRenderPass;
    friend class LightCullingPass;
    friend class DepthPass;

    std::vector<Buffer> m_cameraData = {};
    std::vector<Buffer> m_spheres = {};
    std::vector<Buffer> m_sceneInfoBuffer = {};
    std::vector<CommandList> m_commandBuffers = {};
    std::vector<std::unique_ptr<RenderPass>> m_renderPasses;

    Swapchain m_swapchain = {};

    Mesh m_cube = {};

    Context* m_context = nullptr;
    Envmap* m_envmap = nullptr;

    Texture m_fallbackTexture = {};

    Texture m_lightGridTexture = {};
    std::vector<Buffer> m_lightIndexList = {};

    std::array<RenderTarget, MAX_FRAMES_IN_FLIGHT> m_colorAttachments = {};
    RenderTarget m_depthAttachment = {};
    RenderInfo m_renderInfo = {};

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex = UINT64_MAX;

    Descriptor m_globalDescriptor = {};

    std::vector<VkSemaphore> m_imageAvailableSemaphores = {};
    std::vector<VkSemaphore> m_renderFinishedSemaphores = {};
    std::vector<VkFence> m_inFlightFences = {};
};
} // namespace niji
