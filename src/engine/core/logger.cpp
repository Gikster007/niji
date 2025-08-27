#include "logger.hpp"

#include <imgui.h>

using namespace niji;

void Logger::log_info(const std::string& message)
{
    add_log("[INFO] " + message, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void Logger::log_warning(const std::string& message)
{
    add_log("[WARN] " + message, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
}

void Logger::log_error(const std::string& message)
{
    add_log("[ERROR] " + message, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}

void Logger::add_log(const std::string& message, const glm::vec4& color)
{
    m_entries.push_back({message, color});
}

void Logger::render()
{
    ImGui::Begin("Console");

    //// Scrollable log area
    //if (ImGui::BeginChild("LogRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true))
    //{
        for (auto& entry : m_entries)
        {
            ImVec4 color = {};
            color.x = entry.Color.r;
            color.y = entry.Color.g;
            color.z = entry.Color.b;
            color.w = entry.Color.a;
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(entry.Text.c_str());
            ImGui::PopStyleColor();
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f); // Auto-scroll
    //}
    //ImGui::EndChild();

    ImGui::End();
}
