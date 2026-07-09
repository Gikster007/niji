#pragma once

#include <functional>

namespace niji
{

using PanelFunction = std::function<void()>;

struct Panel
{
    const char* Name = nullptr;
    PanelFunction Function = {};
    bool Visible = true;
};

class Renderer;

class Editor
{
  public:
    Editor() = default;

    void add_debug_menu_panel(const char* name, PanelFunction function);

    void render();

  private:
    friend class ImGuiPass;
    std::vector<Panel> m_panels = {};
};

} // namespace niji