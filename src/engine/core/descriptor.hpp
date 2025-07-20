#pragma once

#include <array>

namespace niji
{

struct DescriptorBinding
{
    VkDescriptorSetLayoutBinding Binding = {};
};

struct DescriptorInfo
{
    std::vector<DescriptorBinding> Bindings = {};
};

class Descriptor
{
  public:
    Descriptor();

  public:
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_set = {};
    VkDescriptorSetLayout m_setLayout = {};
    VkDescriptorPool m_pool = {};
};
} // namespace niji