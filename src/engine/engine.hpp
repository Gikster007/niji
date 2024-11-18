#pragma once

#include <optional>
#include <string>

#include "core/context.hpp"
#include "rendering/renderer.hpp"

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
    Context m_context = {};
    Renderer m_renderer = {};
};
} // namespace niji
