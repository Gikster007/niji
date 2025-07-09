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

    void init(Swapchain& swapchain, VkDescriptorSetLayout& globalLayout);
    void update(Renderer& renderer);
    void record(Renderer& renderer, CommandList& cmd, RenderInfo& info);
    void cleanup();
};

} // namespace niji