#pragma once

#include <optional>
#include <string>

#include "core/context.hpp"
#include "core/ecs.hpp"
#include "core/rendering/renderer.hpp"

namespace niji
{
class Engine
{
  public:
    Engine() = default;
    void run();

  private:
    void init();

    void update();

    void cleanup();

  public:
  private:
    ECS ecs = {};
    Context m_context = {};
};
} // namespace niji
