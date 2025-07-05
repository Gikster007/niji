#pragma once

#include "core/common.hpp"
#include "core/ecs.hpp"
#include "mesh.hpp"
#include "material.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

#include <filesystem>

namespace niji
{
class Model
{
  public:
    Model(std::filesystem::path gltfPath, Entity parent);
    ~Model();

  private:
    void Instantiate(fastgltf::Asset& model, Entity parent = entt::null);
    void InstantiateNode(fastgltf::Asset& model, uint32_t nodeIndex, Entity parent);
    
    void update(float dt);

    void cleanup();

  private:
    friend class Renderer;
    std::vector<niji::Mesh> m_meshes = {};
    std::vector<niji::Material> m_materials = {};
    NijiUBO m_ubo = {};

};
} // namespace niji