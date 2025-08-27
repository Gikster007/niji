#pragma once

#include <string>

#include <glm/glm.hpp>

namespace niji
{

struct LogEntry
{
    std::string Text = {};
    glm::vec4 Color = {};
};

class Logger
{
  public:
    Logger() = default;

    void log_info(const std::string& message);
    void log_warning(const std::string& message);
    void log_error(const std::string& message);

  private:
    void add_log(const std::string& message, const glm::vec4& color = glm::vec4(1.0f));

    void render();

  private:
    friend class Editor;

    std::vector<LogEntry> m_entries;
};
} // namespace niji