#include "model.hpp"

#include "engine.hpp"
#include "core/components/transform.hpp"
#include "core/components/render-components.hpp"
#include "mesh.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/math.hpp>

#include <vk_mem_alloc.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <variant>

#include <iostream>
#include <filesystem>

using namespace niji;

Model::Model(std::filesystem::path gltfPath, Entity parent)
{
    m_gltfPath = gltfPath;
    m_parent = parent;
}

Model::~Model()
{
    cleanup();
}

void Model::Instantiate()
{
    fastgltf::Parser parser{};

    auto data = fastgltf::GltfDataBuffer::FromPath(m_gltfPath);
    if (data.error() != fastgltf::Error::None)
    {
        printf("[Model]: The file couldn't be loaded, or the buffer could not be allocated! \n");
        return;
    }

    auto asset =
        parser.loadGltf(data.get(), m_gltfPath.parent_path(), fastgltf::Options::LoadExternalBuffers);
    if (auto error = asset.error(); error != fastgltf::Error::None)
    {
        printf("[Model]: Some error occurred while reading the buffer, parsing the JSON, or "
               "validating the data! \n");
        return;
    }

    auto& model = asset.get();
    
    for (uint32_t node : model.scenes[0].nodeIndices)
    {
        InstantiateNode(model, m_gltfPath, node, m_parent);
    }
}

void Model::InstantiateNode(fastgltf::Asset& model, std::filesystem::path gltfPath,
                            uint32_t nodeIndex, Entity parent)
{
    auto& node = model.nodes[nodeIndex];
    auto nodeEntity = nijiEngine.ecs.create_entity();

    auto& trans = nijiEngine.ecs.add_component<Transform>(nodeEntity);

    if (parent != entt::null)
    {
        trans.SetParent(parent);
    }

    auto matrix = fastgltf::getTransformMatrix(node);

    trans.SetFromMatrix(glm::make_mat4(matrix.data()));

    // Recurse over child nodes
    for (uint32_t node : node.children)
        InstantiateNode(model, gltfPath, node, nodeEntity);

    if (!node.meshIndex.has_value())
    {
        printf("[Model]: Mesh Index is -1!");
        return;
    }

    auto& mesh = model.meshes[node.meshIndex.value()];
    /*auto& primitive = mesh.primitives[0];

    m_meshes.emplace_back(Mesh(model, primitive));
    m_materials.emplace_back(Material(model, primitive, gltfPath));

    auto& meshComponent = nijiEngine.ecs.add_component<MeshComponent>(nodeEntity);
    meshComponent.Model = this;
    meshComponent.MeshID = node.meshIndex.value();
    meshComponent.MaterialID = primitive.materialIndex.value();*/

    // Loop over all primitives in this mesh
    for (size_t primIndex = 0; primIndex < mesh.primitives.size(); ++primIndex)
    {
        auto& primitive = mesh.primitives[primIndex];

        size_t meshID = m_meshes.size();
        size_t materialID = m_materials.size();

        m_meshes.emplace_back(Mesh(model, primitive));
        m_materials.emplace_back(Material(model, primitive, gltfPath));

        // Alternatively: create child entity per primitive if needed
        Entity primitiveEntity = nijiEngine.ecs.create_entity();
        auto& primTrans = nijiEngine.ecs.add_component<Transform>(primitiveEntity);
        primTrans.SetParent(nodeEntity);
        primTrans.SetFromMatrix(glm::make_mat4(matrix.data()));

        auto& meshComponent = nijiEngine.ecs.add_component<MeshComponent>(primitiveEntity);
        meshComponent.Model = shared_from_this();
        meshComponent.MeshID = meshID;
        meshComponent.MaterialID = materialID;
        
    }
}

void Model::update(float dt)
{

}

void Model::cleanup()
{
    for (int i = 0; i < m_meshes.size(); i++)
    {
        m_meshes[i].cleanup();
    }
    //m_meshes.clear();
    for (int i = 0; i < m_materials.size(); i++)
    {
        m_materials[i].cleanup();
    }
    //m_materials.clear();
}
