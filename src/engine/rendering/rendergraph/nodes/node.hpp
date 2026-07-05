#pragma once

#include <string_view>

#include "rendering/resources/resource_handle.hpp"
#include "rendering/utils/enum_flags.hpp"

namespace niji
{

enum class DependencyStages : uint32_t
{
    None = 0u,
    Compute = 1u << 0u,
    Vertex = 1u << 1u,
    Pixel = 1u << 2u,
};
ENUM_CLASS_FLAGS(DependencyStages);

enum class DependencyUsage : uint32_t
{
    None = 0u,
    // VertexBuffer,
    // IndirectBuffer,
    Readonly,
    ReadWrite,
    // ColorAttachment,
    // Depth,
    // Stencil,
};

struct Dependency
{
    ResourceHandle Resource {};
    DependencyUsage Usage = DependencyUsage::None;
    DependencyStages Stages {};

    Dependency(ResourceHandle resource, DependencyUsage usage, DependencyStages stages);
};

enum class NodeType : uint32_t
{
    Invalid = 0u,
    Compute,
    Raster
};

class Node
{
  public:
    std::string_view m_label {};
    NodeType m_type = NodeType::Invalid;

    // Push Constants
    uint8_t m_pcData[128u] {};
    uint32_t m_rangeOffset = 0u;
    uint32_t m_rangeSize = 0u;
    uint32_t m_rtInjectOffset = UINT32_MAX; // Used to inject correct rt bindless offset

    std::vector<Dependency> m_dependencies {};

    Node() = delete;
    Node(std::string_view label, NodeType type);
};

} // namespace niji