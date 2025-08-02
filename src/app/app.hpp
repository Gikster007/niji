#pragma once

#include "../engine/rendering/model/model.hpp"
#include "core/ecs.hpp"

struct LightSet
{
    std::string Name = {};
    std::vector<entt::entity> PointLights = {};

    // Animation settings
    glm::vec2 Center = {-0.5f, -0.3f};
    float RadiusX = 8.0f;
    float RadiusZ = 2.1f;
    float YPos = 3.4f;
    float Speed = 0.5f;
    bool Animate = true;
    bool DrawDebugSpheres = false;
};

class App : public niji::System
{
  public:
    App();
    ~App();

    void update(float deltaTime) override;
    void render() override;
    void cleanup() override;

    void draw_light_editor();
    void rotate_point_lights();

    void load_lights(std::string path);
    void save_lights(std::string path);
  private:
    std::vector<std::shared_ptr<niji::Model>> m_models = {};
    std::vector<LightSet> m_lightSets = {};
    int m_selectedLightSet = 0;

    niji::Envmap m_envmap = {};
};