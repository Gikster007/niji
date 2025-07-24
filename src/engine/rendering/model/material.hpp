#pragma once

#include <fastgltf/types.hpp>

#include "core/common.hpp"
#include "../renderer.hpp"

namespace niji
{

struct MaterialData
{
    std::optional<Texture> NormalTexture = {};
    std::optional<Texture> OcclusionTexture = {};
    std::optional<Texture> RoughMetallic = {};
    std::optional<Texture> Emissive = {};
    std::optional<Texture> BaseColor = {};
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

    Sampler m_sampler = {};
};

} // namespace niji