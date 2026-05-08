#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "core/descriptor.hpp"
#include "core/context.hpp"
#include "core/envmap.hpp"

#include "core/commandlist.hpp"

#include "model/mesh.hpp"

#include "swapchain.hpp"

#include "resources/pool.hpp"

namespace niji
{

class ResourceBank;

struct Sphere
{
    glm::vec3 Center = {0.0f, 0.0f, 0.0f};
    float Radius = 0.0f;
};

struct RenderInfo
{
    RenderInfo() = default;
    RenderInfo(const VkExtent2D& extent)
    {
        RenderArea.offset = {0, 0};
        RenderArea.extent = extent;
    }

    VkRect2D RenderArea = {{0, 0}, {0, 0}};
    uint32_t LayerCount = 1;

    RenderTargetHandle RenderTarget {};
    TextureHandle ViewportTexture {};

    VkAttachmentLoadOp TargetLoadOp {};
    VkAttachmentStoreOp TargetStoreOp {};
    VkClearValue TargetClearValue {};

    TextureHandle DepthTexture {};

    VkAttachmentLoadOp DepthLoadOp {};
    VkAttachmentStoreOp DepthStoreOp {};
    VkClearValue DepthClearValue {};

    bool HasDepth = false;
    bool PrepareForPresent = false;
};

// TODO: The renderer shouldnt be a system!
class Renderer
{
  public:
    Renderer();
    ~Renderer();

    void init();

    void update(const float dt);
    void render();

    void cleanup();

    //void set_envmap(Envmap& envmap)
    //{
    //    m_envmap = &envmap;
    //}

  private:
    void create_sync_objects();

    void update_uniform_buffer(uint32_t currentImage);

  private:
    friend class CommandList;
    friend class Material;
    friend class Mesh;
    friend class RenderPass;
    friend class ForwardPass;
    friend class ImGuiPass;
    friend class SkyboxPass;
    friend class LineRenderPass;
    friend class LightCullingPass;
    friend class DepthPass;
    friend class Editor;
    
    ResourceBank& m_resourceBank;
    //Swapchain m_swapchain = {};

    //std::vector<Buffer> m_cameraData = {};
    //std::vector<Buffer> m_spheres = {};
    //std::vector<Buffer> m_sceneInfoBuffer = {};
    BufferHandle m_cameraData = {};
    BufferHandle m_spheres = {};
    BufferHandle m_sceneInfoBuffer = {};
    std::vector<CommandList> m_commandBuffers = {};
    std::vector<std::unique_ptr<RenderPass>> m_renderPasses;

    Mesh m_cube = {};

    Context* m_context = nullptr;
    //Envmap* m_envmap = nullptr;

    TextureHandle m_fallbackTexture = {};

    //Texture m_lightGridTexture = {};
    //std::vector<Buffer> m_lightIndexList = {};

    RenderInfo m_renderInfo = {};

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex = UINT64_MAX;

    Descriptor m_globalDescriptor = {};

    std::vector<VkSemaphore> m_imageAvailableSemaphores = {};
    std::vector<VkSemaphore> m_renderFinishedSemaphores = {};
    std::vector<VkFence> m_inFlightFences = {};
};
} // namespace niji
