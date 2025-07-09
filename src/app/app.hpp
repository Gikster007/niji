#pragma once

#include "../engine/rendering/model/model.hpp"
#include "core/ecs.hpp"

class App : public niji::System
{
  public:
    App();
    ~App();

    void update(float deltaTime) override;
    void render() override;
    void cleanup() override;

  private:
    std::vector<std::shared_ptr<niji::Model>> m_models = {};
    niji::Entity m_camera = {};
};