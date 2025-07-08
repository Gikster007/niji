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

  private:
    void update();

  public:
    ECS& ecs;
    Context& m_context;
};
} // namespace niji

extern niji::Engine nijiEngine;
