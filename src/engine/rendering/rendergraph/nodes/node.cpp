#include "node.hpp"

namespace niji
{

Dependency::Dependency(ResourceHandle resource, DependencyUsage usage, DependencyStages stages)
    : Resource(resource), Usage(usage), Stages(stages)
{
}

Node::Node(std::string_view label, NodeType type) : m_label(label), m_type(type)
{
    m_dependencies.reserve(8);
}

} // namespace niji