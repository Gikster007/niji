#include "mesh.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>
#include <vk_mem_alloc.h>

#include "engine.hpp"
#include "core/context.hpp"

#include "tangent_space_wrapper.hpp"

using namespace niji;

void Mesh::cleanup()
{
    m_vertexBuffer.cleanup();
    m_indexBuffer.cleanup();
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

    // Load Normals
    {
        auto normals = primitive.findAttribute("NORMAL");
        if (normals != primitive.attributes.end())
        {
            fastgltf::iterateAccessorWithIndex<glm::vec3>(model,
                                                          model.accessors[(*normals).accessorIndex],
                                                          [&](glm::vec3 n, size_t index) {
                                                              vertices[index].Normal = n;
                                                          });
        }
    }

    // Load Tangents
    {
        auto tangents = primitive.findAttribute("TANGENT");
        if (tangents != primitive.attributes.end())
        {
            fastgltf::iterateAccessorWithIndex<glm::vec4>(model, model.accessors[(*tangents).accessorIndex],
                                                          [&](glm::vec4 t, size_t index) {
                                                              vertices[index].Tangent = t;
                                                          });
        }
        if (tangents->name == "")
        {
            std::vector<uint32_t> indices = {};
            if (m_ushortIndices)
            {
                indices.resize(ushortIndices.size());
                for (size_t i = 0; i < ushortIndices.size(); ++i)
                {
                    indices[i] = static_cast<uint32_t>(ushortIndices[i]);
                }
            }
            else
            {
                indices.resize(uintIndices.size());
                for (size_t i = 0; i < uintIndices.size(); ++i)
                {
                    indices[i] = uintIndices[i];
                }
            }
            std::vector<glm::vec3> p = {};
            std::vector<glm::vec3> n = {};
            std::vector<glm::vec2> t = {};
            std::vector<glm::vec4> tan = {};
            for (int i = 0; i < vertices.size(); i++)
            {
                p.push_back(vertices[i].Pos);
                n.push_back(vertices[i].Normal);
                t.push_back(vertices[i].TexCoord);
            }
            MikkTSpaceTangent::MikktSpaceMesh m = {};
            m.m_indices = indices;
            m.m_normals = n;
            m.m_positions = p;
            m.m_texcoords = t;
            if (!MikkTSpaceTangent::GetTangents(m, tan))
                printf("Failed to generate Tangents! \n");
            for (int i = 0; i < vertices.size(); i++)
            {
                vertices[i].Tangent = tan[i];
                vertices[i].Tangent.w *= -1;
            }
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
