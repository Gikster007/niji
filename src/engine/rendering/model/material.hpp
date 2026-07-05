#pragma once

#include <fastgltf/types.hpp>

#include "core/common.hpp"
#include "../renderer.hpp"

namespace niji
{

struct MaterialData
{
    std::optional<TextureHandle> NormalTexture = {};
    std::optional<TextureHandle> OcclusionTexture = {};
    std::optional<TextureHandle> RoughMetallic = {};
    std::optional<TextureHandle> Emissive = {};
    std::optional<TextureHandle> BaseColor = {};
};

class Material
{
  public:
    Material(fastgltf::Asset& model, fastgltf::Primitive& primitive, std::filesystem::path gltfPath);

    void deinit();

  private:
    friend class Renderer;
    friend class ForwardPass;
    friend class DepthPass;

    MaterialData m_materialData = {};
    MaterialInfo m_materialInfo = {};

    BufferHandle m_data = {};

    SamplerHandle m_sampler = {};
};

} // namespace niji