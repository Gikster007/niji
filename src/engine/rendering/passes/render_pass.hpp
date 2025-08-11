#pragma once

#include <array>

#include "../../core/commandlist.hpp"
#include "../../core/descriptor.hpp"
#include "../../core/common.hpp"

#include "../swapchain.hpp"

namespace niji
{

class RenderPass
{
  public:
    virtual void init(Swapchain& swapchain, Descriptor& globalDescriptor) = 0;
    virtual void update(Renderer& renderer, CommandList& cmd) = 0;
    virtual void record(Renderer& renderer, CommandList& cmd, RenderInfo& info) = 0;
    virtual void cleanup() = 0;

  protected:
    void base_cleanup();

  protected:
    friend class Renderer;

    Pipeline m_pipeline = {};
    std::vector<Buffer> m_passBuffer = {};
    std::string m_name = "Unnamed Pass";

    Descriptor m_passDescriptor = {};
};
} // namespace niji
