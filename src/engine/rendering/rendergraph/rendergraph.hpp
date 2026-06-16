#pragma once

namespace niji
{

class Node;
class ComputeNode;
class RasterNode;

class RenderGraph
{
  public:
    ComputeNode& add_compute_node(std::string_view label, std::string_view shader_path);

    RasterNode& add_raster_node();

private:

    std::vector<Node*> m_nodes {};
};

} // namespace niji