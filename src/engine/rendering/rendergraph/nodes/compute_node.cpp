#include "compute_node.hpp"

namespace niji
{

ComputeNode::ComputeNode(std::string_view label, std::string_view shader_path)
    : Node(label, NodeType::Compute), m_computePath(shader_path)
{
}

ComputeNode::~ComputeNode()
{
}

ComputeNode& ComputeNode::write(ResourceHandle resource)
{
    // Insert the Write Dependency
    m_dependencies.emplace_back(resource, DependencyUsage::ReadWrite, DependencyStages::Compute);
    return *this;
}

ComputeNode& ComputeNode::read(ResourceHandle resource)
{
    // Insert the Read Dependency
    m_dependencies.emplace_back(resource, DependencyUsage::Readonly, DependencyStages::Compute);
    return *this;
}

} // namespace niji