#pragma once

#include "renderer.hpp"

#include <fastgltf/types.hpp>
#include <vk_mem_alloc.h>

namespace niji
{

class Mesh
{
  public:
    Mesh() = default;
    Mesh(fastgltf::Asset& model, fastgltf::Primitive& primitive);

  private:
    VkCommandBuffer begin_single_time_commands();

    void end_single_time_commands(VkCommandBuffer commandBuffer);

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                       VkBuffer& buffer, VmaAllocation& allocation, bool persistent = false);

    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  private:
    friend class Renderer;
    NijiBuffer m_vertexBuffer = {};
    NijiBuffer m_indexBuffer = {};

    uint64_t m_indexCount = 0;
    bool m_ushortIndices = false;
};

} // namespace niji