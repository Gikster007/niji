#pragma once

#include <optional>
#include <string>

#include "core/context.hpp"
#include "core/ecs.hpp"

namespace niji
{
class Engine
{
  public:
    Engine();
    ~Engine();
    void init();
    void run();
    void cleanup();

    void add_line(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& color);

    void add_sphere(const glm::vec3& center, const float& radius, const glm::vec3& color,
                    int segments);

  private:
    void update();

  public:
    ECS& ecs;
    Context& m_context;

  private:
    friend class LineRenderPass;
    std::vector<DebugLine> m_debugLines = {};
};
} // namespace niji

extern niji::Engine nijiEngine;
