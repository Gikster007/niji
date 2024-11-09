#pragma once

#include <optional>
#include <string>

#include "core/context.hpp"
#include "rendering/renderer.hpp"

namespace molten
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
} // namespace molten
