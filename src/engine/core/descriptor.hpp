#pragma once

#include <variant>
#include <array>

#include "common.hpp"

namespace niji
{

using DescriptorResource =
    std::variant<std::monostate, std::array<Buffer, MAX_FRAMES_IN_FLIGHT>*, NijiTexture*>;
struct DescriptorBinding
{
    enum class BindType
    {
        NONE,
        UBO,
        SAMPLER,
        TEXTURE

    } Type = BindType::NONE;

    enum class BindStage
    {
        NONE,
        VERTEX_SHADER,
        FRAGMENT_SHADER,
        ALL_GRAPHICS
    } Stage = BindStage::NONE;

    uint32_t Count = {};
    const VkSampler* Sampler = nullptr;

    DescriptorResource Resource = {};
};

struct DescriptorInfo
{
    std::vector<DescriptorBinding> Bindings = {};
    bool IsPushDescriptor = false;
};

class Descriptor
{
  public:
    Descriptor() = default;
    Descriptor(DescriptorInfo& info);

    void cleanup() const;

  public:
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_set = {};
    VkDescriptorSetLayout m_setLayout = {};
    VkDescriptorPool m_pool = {};

  private:
    DescriptorInfo m_info = {};
};
} // namespace niji