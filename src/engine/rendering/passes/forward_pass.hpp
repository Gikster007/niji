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
    void update(Renderer& renderer, CommandList& cmd);
    void record(Renderer& renderer, CommandList& cmd, RenderInfo& info);
    void cleanup();

  private:
    DebugSettings m_debugSettings = {};
    std::vector<Buffer> m_pointLightBuffer = {};
    Texture m_depthTexture = {};
};

} // namespace niji