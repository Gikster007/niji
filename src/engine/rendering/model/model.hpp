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
class Model : public std::enable_shared_from_this<Model>
{
  public:
    Model(std::filesystem::path gltfPath, Entity parent);
    ~Model();
    void Instantiate();

  private:
    void InstantiateNode(fastgltf::Asset& model, std::filesystem::path gltfPath,
                         uint32_t nodeIndex, Entity parent);
    
    void update(float dt);

    void cleanup();

  private:
    friend class Renderer;
    friend class ForwardPass;
    friend class SkyboxPass;

    std::filesystem::path m_gltfPath = {};
    Entity m_parent = {};

    std::vector<niji::Mesh> m_meshes = {};
    std::vector<niji::Material> m_materials = {};

};
} // namespace niji