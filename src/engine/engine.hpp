#pragma once

#include "core/common.hpp"

namespace niji
{

class ECS;
class Context;
class Editor;
class Logger;
class Renderer;

class Engine
{
  public:
    Engine();
    ~Engine();
    void init();
    void run();
    void deinit();

    void add_line(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& color);

    void add_sphere(const glm::vec3& center, const float& radius, const glm::vec3& color,
                    int segments);

  private:
    void update();

  public:
    ECS& ecs;
    Context& m_context;
    Editor& m_editor;
    Logger& m_logger;
    Renderer& m_renderer;

  private:
    friend class LineRenderPass;

    std::vector<DebugLine> m_debugLines = {};
};
} // namespace niji

extern niji::Engine nijiEngine;
