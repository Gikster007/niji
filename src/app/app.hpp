#pragma once

#include <optional>
#include <string>

#include "core/context.hpp"
#include "rendering/renderer.hpp"

namespace molten
{
class App
{
  public:
    App() = default;
    void run();

  private:
    void init_vulkan();

    void main_loop();

    void cleanup();

  public:
  private:
    Context m_context = {};
    Renderer m_renderer;
};
} // namespace molten
