#pragma once

#pragma once

#include "render_pass.hpp"

namespace niji
{

struct DispatchParams
{
    glm::mat4 InverseProjection = {};
    glm::u32vec3 numThreadGroups = {};
    glm::u32vec3 numThreads = {};
    glm::vec2 ScreenDimensions = {};
    glm::vec2 _pad0;
};

struct Plane
{
    glm::vec3 Normal = {};
    float Distance = {};
};

struct Frustum
{
    Plane Planes[4];
};

struct LightIndexCounter
{
    uint32_t counter = {};
};

struct LightIndexList
{
    uint32_t counter = {};
};

constexpr uint16_t GROUP_SIZE = 16;
constexpr uint16_t MAX_LIGHTS_PER_TILE = 1024;

class LightCullingPass final : public RenderPass
{
  public:
    LightCullingPass()
    {
    }

    void init(Swapchain& swapchain, Descriptor& globalDescriptor);
    void update_impl(Renderer& renderer, CommandList& cmd);
    void record(Renderer& renderer, CommandList& cmd, RenderInfo& info);
    void cleanup();

    void debug_panel();

    private:
    void on_window_resize();

  private:
    Descriptor m_lightCullingDescriptor = {};

    std::vector<Buffer> m_dispatchParams = {};
    std::vector<Buffer> m_frustums = {};

    std::vector<Buffer> m_lightIndexCounter = {};
    //std::vector<Buffer> m_lightIndexList = {};
    //Texture m_lightGridTexture = {};
    Texture m_depthTexture = {};

    glm::u32vec2 m_totalThreads = {};
    glm::u32vec2 m_totalThreadGroups = {};

    int m_winWidth = 0;
    int m_winHeight = 0;
};

} // namespace niji