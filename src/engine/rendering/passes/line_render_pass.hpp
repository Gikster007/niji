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
    void update(Renderer& renderer);
    void record(Renderer& renderer, CommandList& cmd, RenderInfo& info);
    void cleanup();

  private:
    Buffer m_vertexBuffer = {};
};

} // namespace niji