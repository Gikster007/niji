#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "core/context.hpp"
#include "core/envmap.hpp"

#include "model/mesh.hpp"

#include "swapchain.hpp"

#include "resources/pool.hpp"

namespace niji
{

class ResourceBank;
class RenderGraph;

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

class Renderer
{
  public:
    Renderer();
    ~Renderer();

    void init();

    void update(const float dt);
    void render();

    void deinit();

    //void set_envmap(Envmap& envmap)
    //{
    //    m_envmap = &envmap;
    //}
    SamplerHandle m_globalSampler = {};

  private:
    void update_uniform_buffer();

  private:
    friend class Material;
    friend class Mesh;
    friend class Editor;
    friend class PipelineCache;
    friend class RenderGraph;
    friend class ImGUI;
    
    ResourceBank& m_resourceBank;
    RenderGraph& m_renderGraph;

    BufferHandle m_cameraData = {};
    BufferHandle m_spheres = {};
    BufferHandle m_sceneInfoBuffer = {};

    Mesh m_cube = {};

    Context* m_context = nullptr;
    //Envmap* m_envmap = nullptr;

    TextureHandle m_fallbackTexture = {};

    //Texture m_lightGridTexture = {};
    //std::vector<Buffer> m_lightIndexList = {};

    RenderInfo m_renderInfo = {};
};
} // namespace niji
