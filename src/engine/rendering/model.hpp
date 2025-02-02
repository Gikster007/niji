#pragma once

#include "core/ecs.hpp"
#include "mesh.hpp"


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

  private:
    friend class Renderer;
    std::vector<niji::Mesh> m_meshes = {};
    // std::vector<bee::Material> m_materials = {};

};
} // namespace niji