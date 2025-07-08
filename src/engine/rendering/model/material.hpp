#pragma once

#include "core/common.hpp"
#include "../renderer.hpp"

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
    Material(fastgltf::Asset& model, fastgltf::Primitive& primitive);

    private:
    void cleanup();

  private:
    friend class ForwardPass;

    MaterialData m_materialData = {};
    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> m_data = {};

    VkSampler m_sampler = {};
};

} // namespace niji