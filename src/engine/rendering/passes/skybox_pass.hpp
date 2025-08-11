#pragma once

#include "render_pass.hpp"

namespace niji
{
class SkyboxPass final : public RenderPass
{
  public:
    SkyboxPass()
    {
    }

    void init(Swapchain& swapchain, Descriptor& globalDescriptor);
    void update(Renderer& renderer, CommandList& cmd);
    void record(Renderer& renderer, CommandList& cmd, RenderInfo& info);
    void cleanup();

  private:
    Sampler m_sampler = {};
};
} // namespace niji