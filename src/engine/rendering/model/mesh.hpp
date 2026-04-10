#pragma once

#include <fastgltf/types.hpp>
#include <glm/fwd.hpp>

#include "rendering/resources/buffer.hpp"

namespace niji
{

class Mesh
{
  public:
    Mesh() = default;
    Mesh(fastgltf::Asset& model, fastgltf::Primitive& primitive);

    Mesh::Mesh(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices);

    void cleanup();

  private:
    friend class Renderer;
    friend class ForwardPass;
    friend class SkyboxPass;
    friend class DepthPass;

    Buffer m_vertexBuffer = {};
    Buffer m_indexBuffer = {};

    uint64_t m_indexCount = 0;
    bool m_ushortIndices = false;
};

} // namespace niji