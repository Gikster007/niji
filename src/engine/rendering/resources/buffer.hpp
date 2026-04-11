#pragma once

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

#include "rendering/utils/enum_flags.hpp"

namespace niji
{

enum class BufferUsage
{
    Invalid = 0u,
    TransferDst = 1u << 1u, // Can Be Written to by a Transfer Command
    TransferSrc = 1u << 2u, // Can be Read from by a Transfer Command
    Vertex = 1u << 3u,      // Vertex Buffer
    Index = 1u << 4u,       // Index Buffer
    Uniform = 1u << 5u,     // Uniform Buffer
    Storage = 1u << 6u      // Storage BUffer
};

struct BufferDesc
{
    uint64_t Size = 0u;

    std::string Name = "Unknown Buffer";

    BufferUsage Usage = BufferUsage::Invalid;
};
ENUM_CLASS_FLAGS(BufferUsage);

// Buffer Resource
struct Buffer
{
    Buffer() = default;

    // void cleanup();

    VkBuffer Object = {};
    VmaAllocation Allocation = {};

    BufferDesc Desc = {};

    VkDeviceAddress Address = {};
};

} // namespace niji