#include "editor.hpp"

#include <imgui.h>

#include "engine.hpp"

using namespace niji;

void Editor::add_debug_menu_panel(const char* name, PanelFunction function)
{
    // Default All Added Panels to Visible;
    m_panels.push_back({name, function, true});
}

void Editor::render()
{
    // Docking
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    // Main menu bar
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Debug"))
            {
                for (auto& panel : m_panels)
                {
                    panel.Function();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    // Engine Console
    {
        nijiEngine.m_logger.render();
    }
}