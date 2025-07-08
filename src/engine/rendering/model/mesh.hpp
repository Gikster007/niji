#pragma once

#include "core/common.hpp"
#include "../renderer.hpp"

#include <fastgltf/types.hpp>

namespace niji
{

class Mesh
{
  public:
    Mesh(fastgltf::Asset& model, fastgltf::Primitive& primitive);

    void cleanup();

  private:
    friend class Renderer;
    friend class ForwardPass;

    Buffer m_vertexBuffer = {};
    Buffer m_indexBuffer = {};

    uint64_t m_indexCount = 0;
    bool m_ushortIndices = false;
};

} // namespace niji