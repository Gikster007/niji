#pragma once

#include "node.hpp"

namespace niji
{

class ComputeNode : public Node
{
  public: 
    ComputeNode() = default;
    ComputeNode(std::string_view label, std::string_view shader_path);
    ~ComputeNode();

    // Add a Bindable Resource as an Output
    ComputeNode& write(ResourceHandle resource);

    // Add a Bindable Resource as an Input
    ComputeNode& read(ResourceHandle resource);

    // Set the Thread Group Size for this Node
    inline ComputeNode& group_size(uint32_t x, uint32_t y = 1u, uint32_t z = 1u)
    {
        m_groupX = x;
        m_groupY = y;
        m_groupZ = z;
        return *this;
    }

    // Set the Work Size for this Node (this will be divided by the `group_size` to get the dispatch size)
    inline ComputeNode& work_size(uint32_t x, uint32_t y = 1u, uint32_t z = 1u)
    {
        m_workX = x;
        m_workY = y;
        m_workZ = z;
        return *this;
    }

    public:
    // Compute shader file path 
    std::string_view m_computePath {};

    // Thread group size
    uint32_t m_groupX = 1u;
    uint32_t m_groupY = 1u;
    uint32_t m_groupZ = 1u;
    // Work size
    uint32_t m_workX = 1u;
    uint32_t m_workY = 1u;
    uint32_t m_workZ = 1u;
};

} // namespace niji