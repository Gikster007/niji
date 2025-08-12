#pragma once

#include "render_pass.hpp"

namespace niji
{

class DepthPass final : public RenderPass
{
  public:
    DepthPass()
    {
    }

    void init(Swapchain& swapchain, Descriptor& globalDescriptor);
    void update_impl(Renderer& renderer, CommandList& cmd);
    void record(Renderer& renderer, CommandList& cmd, RenderInfo& info);
    void cleanup();

  private:
};

} // namespace niji