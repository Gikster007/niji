#pragma once

#include "render_pass.hpp"

namespace niji
{

class ImGuiPass final : public RenderPass
{
  public:
    ImGuiPass()
    {
    }

    void init(Swapchain& swapchain, VkDescriptorSetLayout& globalLayout);
    void update(Renderer& renderer);
    void record(Renderer& renderer, CommandList& cmd, RenderInfo& info);
    void cleanup();

  private:
    VkDescriptorPool m_imguiDescriptorPool = {};
};

} // namespace niji