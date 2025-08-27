#pragma once

#include <unordered_map>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <array>

#include "../../core/commandlist.hpp"
#include "../../core/descriptor.hpp"
#include "../../core/common.hpp"

#include "../swapchain.hpp"

namespace fs = std::filesystem;

namespace niji
{

class ShaderWatcher
{
  public:
    // Start tracking a file (store initial write time)
    void trackFile(const std::string& path)
    {
        timestamps[path] = fs::last_write_time(path);
    }

    // Check if a tracked file has changed since last check
    bool hasChanged(const std::string& path)
    {
        auto currentWriteTime = fs::last_write_time(path);
        if (timestamps.find(path) != timestamps.end() && timestamps[path] != currentWriteTime)
        {
            timestamps[path] = currentWriteTime;
            return true;
        }
        return false;
    }

  private:
    std::unordered_map<std::string, fs::file_time_type> timestamps;
};

class RenderPass
{
  public:
    virtual void init(Swapchain& swapchain, Descriptor& globalDescriptor) = 0;
    void update(Renderer& renderer, CommandList& cmd);
    virtual void record(Renderer& renderer, CommandList& cmd, RenderInfo& info) = 0;
    virtual void cleanup() = 0;

  protected:
    virtual void update_impl(Renderer& renderer, CommandList& cmd) = 0;
    void base_cleanup();

    void add_shader(const std::string& path, ShaderType shaderType)
    {
        if (shaderType == ShaderType::NONE)
            throw std::runtime_error("Invalid Shader Type");
        else if (shaderType == ShaderType::FRAG_AND_VERT)
            m_vertFrag = Shader(path, shaderType);
        else if (shaderType == ShaderType::COMPUTE)
            m_compute = Shader(path, shaderType);

        m_shaderWatcher.trackFile(path);
    }

  protected:
    friend class Renderer;
    friend class Editor;

    std::unordered_map<std::string, Pipeline> m_pipelines = {};
    std::vector<Buffer> m_passBuffer = {};
    std::string m_name = "Unnamed Pass";

    Descriptor m_passDescriptor = {};

    ShaderWatcher m_shaderWatcher = {};
    Shader m_vertFrag = {};
    Shader m_compute = {};
};
} // namespace niji
