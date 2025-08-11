#pragma once 

#include "render_pass.hpp"

namespace niji
{

class LineRenderPass final : public RenderPass
{
  public:
    LineRenderPass()
    {
    }

    void init(Swapchain& swapchain, Descriptor& globalDescriptor);
    void update(Renderer& renderer, CommandList& cmd);
    void record(Renderer& renderer, CommandList& cmd, RenderInfo& info);
    void cleanup();

  private:
    Buffer m_vertexBuffer = {};
};

} // namespace niji