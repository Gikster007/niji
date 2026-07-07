#pragma once

#include "rendering/resources/resource_handle.hpp"

namespace niji
{

class ImGUI
{
  public:
    void init(RenderTargetHandle rt);

    void deinit() const;

    void render(VkCommandBuffer cmd);

    void new_frame();

  private:
    VkDescriptorPool m_descPool {};
    VkSampler m_bilinearSampler {};
};

} // namespace niji