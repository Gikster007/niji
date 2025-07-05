#include "mesh.hpp"

#include "engine.hpp"
#include "core/context.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>
#include <vk_mem_alloc.h>

using namespace niji;

void Mesh::cleanup() const
{
    vmaDestroyBuffer(nijiEngine.m_context.m_allocator, m_indexBuffer.Handle,
                     m_indexBuffer.BufferAllocation);

    vmaDestroyBuffer(nijiEngine.m_context.m_allocator, m_vertexBuffer.Handle,
                     m_vertexBuffer.BufferAllocation);
}

Mesh::Mesh(fastgltf::Asset& model, fastgltf::Primitive& primitive)
{
    std::vector<uint32_t> uintIndices = {};
    std::vector<uint16_t> ushortIndices = {};
    std::vector<Vertex> vertices = {};

    // Load Indices
    {
        fastgltf::Accessor& indexAccessor = model.accessors[primitive.indicesAccessor.value()];

        switch (indexAccessor.componentType)
        {
        case fastgltf::ComponentType::UnsignedShort:
            m_ushortIndices = true;
            ushortIndices.reserve(ushortIndices.size() + indexAccessor.count);

            fastgltf::iterateAccessor<std::uint16_t>(model, indexAccessor, [&](std::uint16_t idx) {
                ushortIndices.push_back(idx);
            });

            m_indexCount = ushortIndices.size();
            break;

        case fastgltf::ComponentType::UnsignedInt:
            uintIndices.reserve(uintIndices.size() + indexAccessor.count);

            fastgltf::iterateAccessor<std::uint32_t>(model, indexAccessor, [&](std::uint32_t idx) {
                uintIndices.push_back(idx);
            });

            m_indexCount = uintIndices.size();
            break;

        default:
            break;
        }

        
    }

    // Load Vertices
    {
        fastgltf::Accessor& posAccessor =
            model.accessors[primitive.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + posAccessor.count);
        fastgltf::iterateAccessorWithIndex<glm::vec3>(model, posAccessor,
                                                      [&](glm::vec3 v, size_t index) {
                                                          Vertex newVertex;
                                                          newVertex.Pos = v;
                                                          newVertex.Color = glm::vec4{1.0f};
                                                          newVertex.TexCoord =
                                                              glm::vec2(0.0f, 0.0f);
                                                          vertices[index] = newVertex;
                                                      });
    }

    // Load UVs
    {
        auto uv = primitive.findAttribute("TEXCOORD_0");
        if (uv != primitive.attributes.end())
        {

            fastgltf::iterateAccessorWithIndex<glm::vec2>(model,
                                                          model.accessors[(*uv).accessorIndex],
                                                          [&](glm::vec2 v, size_t index) {
                                                              vertices[index].TexCoord = v;
                                                          });
        }
    }

    // Load Vertex Colors
    {
        auto colors = primitive.findAttribute("COLOR_0");
        if (colors != primitive.attributes.end())
        {

            fastgltf::iterateAccessorWithIndex<glm::vec4>(model,
                                                          model.accessors[(*colors).accessorIndex],
                                                          [&](glm::vec4 v, size_t index) {
                                                              vertices[index].Color =
                                                                  glm::vec3(v.x, v.y, v.z);
                                                          });
        }
    }

    // Create Vertex Buffer
    {
        BufferDesc desc = {};
        desc.IsPersistent = false;
        desc.Size = sizeof(vertices[0]) * vertices.size();
        desc.Usage = BufferDesc::BufferUsage::Vertex;
        desc.Name = "Vertex Buffer";
        m_vertexBuffer = Buffer(desc, vertices.data());
    }

    // Create Index Buffer
    {
        BufferDesc desc = {};
        desc.IsPersistent = false;
        desc.Usage = BufferDesc::BufferUsage::Index;
        desc.Name = "Index Buffer";

        void* data = nullptr;

        if (m_ushortIndices) // Unsigned Short
        {
            desc.Size = sizeof(ushortIndices[0]) * ushortIndices.size();
            
            data = ushortIndices.data();
        }
        else // Unsigned Int
        {
            desc.Size = sizeof(uintIndices[0]) * uintIndices.size();

            data = uintIndices.data();
        }

        m_indexBuffer = Buffer(desc, data);
    }
}
