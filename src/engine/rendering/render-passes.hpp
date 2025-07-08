#pragma once

#include <array>

#include "../core/commandlist.hpp"
#include "../core/common.hpp"

#include "swapchain.hpp"

namespace niji
{

class RenderPass
{
  public:
    virtual void init(Swapchain& swapchain, VkDescriptorSetLayout& globalLayout) = 0;
    virtual void update(Renderer& renderer) = 0;
    virtual void record(Renderer& renderer, CommandList& cmd) = 0;
    virtual void cleanup() = 0;

  protected:
    void base_cleanup();

  protected:
    Pipeline m_pipeline = {};
    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> m_passBuffer = {};

    VkDescriptorSetLayout m_passDescriptorSetLayout = {};
};

class ForwardPass final : public RenderPass
{
  public:
    ForwardPass()
    {
    }

    void init(Swapchain& swapchain, VkDescriptorSetLayout& globalLayout);
    void update(Renderer& renderer);
    void record(Renderer& renderer, CommandList& cmd);
    void cleanup();
};
} // namespace niji
