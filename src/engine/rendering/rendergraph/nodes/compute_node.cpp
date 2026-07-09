#include "compute_node.hpp"

#include <cassert>

namespace niji
{

// Round up to a multiple of 4
inline uint32_t align_pc_size(uint32_t size)
{
    return (size + 3u) & ~3u;
}

ComputeNode::ComputeNode(std::string_view label, std::string_view shaderPath)
    : Node(label, NodeType::Compute), m_computePath(shaderPath)
{
}

ComputeNode::~ComputeNode()
{
}

ComputeNode& ComputeNode::write(ResourceHandle resource, uint32_t pcOffset)
{
    // Insert the Write Dependency
    m_dependencies.emplace_back(resource, DependencyUsage::ReadWrite, DependencyStages::Compute);
    
    if (resource.Type == ResourceType::RenderTarget)
    {
        m_rtInjectOffset = pcOffset;
        const uint32_t needed = align_pc_size(pcOffset + sizeof(uint32_t));
        if (needed > m_rangeSize)
            m_rangeSize = needed;
    }
    
    return *this;
}

ComputeNode& ComputeNode::read(ResourceHandle resource)
{
    // Insert the Read Dependency
    m_dependencies.emplace_back(resource, DependencyUsage::Readonly, DependencyStages::Compute);

    return *this;
}

ComputeNode& ComputeNode::push_constants(void* data, uint32_t offset, uint32_t size)
{
    assert(offset + size <= sizeof(m_pcData) && "push constants exceed 128 bytes");
    memcpy(m_pcData + offset, data, size);
    m_rangeOffset = offset;

    const uint32_t needed = offset + size;
    if (needed > m_rangeSize)
        m_rangeSize = needed;
    
    return *this;
}

} // namespace niji