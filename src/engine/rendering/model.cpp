#include "model.hpp"

#include "engine.hpp"
#include "core/components/transform.hpp"
#include "core/components/render-components.hpp"
#include "mesh.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/math.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <variant>

#include <iostream>
#include <filesystem>

using namespace niji;

Model::Model(std::filesystem::path gltfPath, Entity parent)
{
    fastgltf::Parser parser{};

    auto data = fastgltf::GltfDataBuffer::FromPath(gltfPath);
    if (data.error() != fastgltf::Error::None)
    {
        printf("[Model]: The file couldn't be loaded, or the buffer could not be allocated! \n");
        return;
    }

    auto asset =
        parser.loadGltf(data.get(), gltfPath.parent_path(), fastgltf::Options::LoadExternalBuffers);
    if (auto error = asset.error(); error != fastgltf::Error::None)
    {
        printf("[Model]: Some error occurred while reading the buffer, parsing the JSON, or "
               "validating the data! \n");
        return;
    }

    // Create UBO
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    m_ubo.UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_ubo.UniformBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);
    m_ubo.UniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        nijiEngine.m_context.create_buffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, m_ubo.UniformBuffers[i],
                                           m_ubo.UniformBuffersAllocations[i], true);

        vmaMapMemory(nijiEngine.m_context.m_allocator, m_ubo.UniformBuffersAllocations[i],
                     &m_ubo.UniformBuffersMapped[i]);
    }

    this->Instantiate(asset.get(), parent);
}

Model::~Model()
{
}

void Model::Instantiate(fastgltf::Asset& model, Entity parent)
{
    for (uint32_t node : model.scenes[0].nodeIndices)
    {
        InstantiateNode(model, node, parent);
    }
}

void Model::InstantiateNode(fastgltf::Asset& model, uint32_t nodeIndex, Entity parent)
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
        InstantiateNode(model, node, parent);

    if (!node.meshIndex.has_value())
    {
        printf("[Model]: Mesh Index is -1!");
        return;
    }

    auto& mesh = model.meshes[node.meshIndex.value()];
    auto& primitive = mesh.primitives[0];

    m_meshes.emplace_back(Mesh(model, primitive));
    m_materials.emplace_back(Material(model, primitive, m_ubo));

    auto& meshComponent = nijiEngine.ecs.add_component<MeshComponent>(nodeEntity);
    meshComponent.Model = this;
    meshComponent.MeshID = node.meshIndex.value();
    meshComponent.MaterialID = primitive.materialIndex.value();
}

void Model::update(float dt)
{

}

void Model::cleanup()
{
    nijiEngine.m_context.cleanup_ubo(m_ubo);
}
