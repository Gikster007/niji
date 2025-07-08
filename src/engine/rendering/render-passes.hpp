#pragma once

#include <array>

#include "../core/commandlist.hpp"
#include "../core/common.hpp"

namespace niji
{

class RenderPass
{
  public:
    virtual void init(VkExtent2D swapChainExtent, VkDescriptorSetLayout& globalLayout) = 0;
    virtual void update(Renderer& renderer) = 0;
    virtual void record(Renderer& renderer, CommandList& cmd) = 0;

  protected:
    Pipeline m_pipeline = {};
    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> m_passBuffer = {};

    VkDescriptorSetLayout m_passDescriptorSetLayout = {};
    VkDescriptorSetLayout m_materialDescriptorSetLayout = {};
};

class ForwardPass final : public RenderPass
{
  public:
    ForwardPass()
    {
    }

    void init(VkExtent2D swapChainExtent, VkDescriptorSetLayout& globalLayout);
    void update(Renderer& renderer);
    void record(Renderer& renderer, CommandList& cmd);
};
} // namespace niji
