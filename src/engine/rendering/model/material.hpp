#pragma once

#include <fastgltf/types.hpp>

#include "core/common.hpp"
#include "../renderer.hpp"

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
    Material(fastgltf::Asset& model, fastgltf::Primitive& primitive,
             std::filesystem::path gltfPath);

    void cleanup();

  private:
    friend class Renderer;
    friend class ForwardPass;

    MaterialData m_materialData = {};
    MaterialInfo m_materialInfo = {};
    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> m_data = {};

    VkSampler m_sampler = {};
};

} // namespace niji