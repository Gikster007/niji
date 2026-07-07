#pragma once

#include <string_view>

#include "rendering/pipeline_cache.hpp"

#include "rendering/resources/resource_handle.hpp"

#include "rendering/utils/imgui.hpp"

namespace niji
{

class Node;
class ComputeNode;
class RasterNode;

// Resources Used by the Render Graph for a Frame In-Flight
struct FrameResources
{
    VkCommandBuffer Cmd {};
    VkSemaphore Semaphore {};
    VkFence Fence {};
};

class RenderGraph
{
  public:
    void init();
    void deinit();

    // Resets the Render Graph
    void new_frame();

    // Returns Current Frame's Render Graph Resources
    FrameResources& current_frame();

    // Updates the Frame Resources Ring Buffer Index (call at the end of the execute() function)
    void next_frame();

    ComputeNode& add_compute_node(std::string_view label, std::string_view shader_path);

    RasterNode& add_raster_node();

    void set_render_target(RenderTargetHandle& rt);

    void execute();

  public:
    // Max Nr of Frames in Flight
    uint32_t m_maxFrames = 2u;

  private:
    friend class Renderer;

    RenderTargetHandle m_renderTarget {};

    PipelineCache m_pipelineCache {};

    std::vector<Node*> m_nodes {};

    // Render Graph Frame Resources Ring Buffer
    FrameResources* m_resources {};
    // Frame Resources Ring Buffer Index
    uint32_t m_currentFrame = 0u;

    ImGUI m_imgui {};
};

} // namespace niji