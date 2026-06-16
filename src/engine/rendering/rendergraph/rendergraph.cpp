#include "rendergraph.hpp"

#include "nodes/compute_node.hpp"

namespace niji
{

ComputeNode& RenderGraph::add_compute_node(std::string_view label, std::string_view shader_path)
{
    ComputeNode* node = new ComputeNode(label, shader_path);
    m_nodes.emplace_back((Node*)node);
    return *node;
}

} // namespace niji