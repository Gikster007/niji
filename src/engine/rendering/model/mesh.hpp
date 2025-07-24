#pragma once

#include "core/common.hpp"
//#include "../renderer.hpp"

#include <fastgltf/types.hpp>

namespace niji
{

class Mesh
{
  public:
    Mesh() = default;
    Mesh(fastgltf::Asset& model, fastgltf::Primitive& primitive);

    Mesh::Mesh(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices)
    {
        m_indexCount = indices.size();
        m_ushortIndices = false;
        //m_vertexStride = sizeof(VertexType);

        // Vertex Buffer
        {
            BufferDesc desc = {};
            desc.IsPersistent = false;
            desc.Size = sizeof(glm::vec3) * vertices.size();
            desc.Usage = BufferDesc::BufferUsage::Vertex;
            desc.Name = "Custom Vertex Buffer";

            m_vertexBuffer = Buffer(desc, vertices.data());
        }

        // Index Buffer
        {
            BufferDesc desc = {};
            desc.IsPersistent = false;
            desc.Size = sizeof(uint32_t) * indices.size();
            desc.Usage = BufferDesc::BufferUsage::Index;
            desc.Name = "Custom Index Buffer";

            m_indexBuffer = Buffer(desc, indices.data());
        }
    }

    void cleanup();

  private:
    friend class Renderer;
    friend class ForwardPass;
    friend class SkyboxPass;

    Buffer m_vertexBuffer = {};
    Buffer m_indexBuffer = {};

    uint64_t m_indexCount = 0;
    bool m_ushortIndices = false;
};

} // namespace niji