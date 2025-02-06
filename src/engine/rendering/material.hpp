#pragma once

#include "core/common.hpp"
#include "renderer.hpp"

#include <fastgltf/types.hpp>

namespace niji
{

struct MaterialData
{
    std::optional<NijiTexture> NormalTexture = {};
    std::optional<NijiTexture> OcclusionTexture = {};
    std::optional<NijiTexture> RoughMetallic = {};
    std::optional<NijiTexture> Emissive = {};
    std::optional<NijiTexture> BaseColor = {};
};

class Material
{
  public:
    Material(fastgltf::Asset& model, fastgltf::Primitive& primitive, NijiUBO& ubo);

    private:
    void cleanup();

  private:
    friend class Renderer;
    MaterialData m_materialData = {};

    VkSampler m_sampler = {};

    VkDescriptorPool m_descriptorPool = {};
    std::vector<VkDescriptorSet> m_descriptorSets = {};
    VkDescriptorSetLayout m_descriptorSetLayout = {};
};

} // namespace niji