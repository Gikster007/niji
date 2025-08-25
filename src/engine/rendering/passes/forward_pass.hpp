#pragma once

#include "render_pass.hpp"

namespace niji
{

class ForwardPass final : public RenderPass
{
  public:
    ForwardPass()
    {
    }

    void init(Swapchain& swapchain, Descriptor& globalDescriptor);
    void update_impl(Renderer& renderer, CommandList& cmd);
    void record(Renderer& renderer, CommandList& cmd, RenderInfo& info);
    void cleanup();

    void debug_panel();
  private:
    DebugSettings m_debugSettings = {};
    std::vector<Buffer> m_pointLightBuffer = {};
    Texture m_depthTexture = {};
    Sampler m_pointSampler = {};
};

} // namespace niji