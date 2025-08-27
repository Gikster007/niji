#include "editor.hpp"

#include <imgui.h>

#include "rendering/passes/render_pass.hpp"
#include "rendering/renderer.hpp"
#include "engine.hpp"

using namespace niji;

void Editor::add_debug_menu_panel(const char* name, PanelFunction function)
{
    // Default All Added Panels to Visible;
    m_panels.push_back({name, function, true});
}

void Editor::render(Renderer& renderer)
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

    // Shader Editor
    {
        ImGui::Begin("Shaders");

        // TODO: ADD TO INIT FUNCTION
        // Get All Render Passes
        auto& passes = renderer.m_renderPasses;

        // Get All Shaders
        std::vector<std::shared_ptr<Shader>> shaders = {};
        for (const auto& pass : passes)
        {
            if (pass->m_vertFrag.Type != ShaderType::NONE)
                shaders.push_back(std::make_shared<Shader>(pass->m_vertFrag));

            if (pass->m_compute.Type != ShaderType::NONE)
                shaders.push_back(std::make_shared<Shader>(pass->m_compute));
        }

        for (const auto& shader : shaders)
        {
            ImGui::Selectable(shader->Source.c_str(), false);
        }

        ImGui::End();
    }
}