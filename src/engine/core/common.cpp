#include "common.hpp"

#include <stdexcept>
#include <vk_mem_alloc.h>

#include "engine.hpp"

using namespace niji;

Buffer::Buffer(BufferDesc& desc, void* data)
{
    VkBuffer stagingBuffer = {};
    VmaAllocation stagingBufferAllocation = {};
    nijiEngine.m_context.create_buffer(desc.Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer,
                                       stagingBufferAllocation);

    void* dataStaged = nullptr;
    vmaMapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation, &dataStaged);
    memcpy(dataStaged, data, (size_t)desc.Size);
    vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation);

    VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    switch (desc.Usage)
    {
    case BufferDesc::BufferUsage::Invalid:
        throw std::runtime_error("Failed to Create Buffer. Invalid Usage Set!");
        break;
    case BufferDesc::BufferUsage::Vertex:
        usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        break;
    case BufferDesc::BufferUsage::Index:
        usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;
    default:
        break;
    }
    
    nijiEngine.m_context.create_buffer(desc.Size,
                                       usageFlags,
                                       VMA_MEMORY_USAGE_GPU_ONLY, Handle, BufferAllocation, desc.IsPersistent);
    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_BUFFER, Handle, desc.Name);

    nijiEngine.m_context.copy_buffer(stagingBuffer, Handle, desc.Size);

    vmaDestroyBuffer(nijiEngine.m_context.m_allocator, stagingBuffer, stagingBufferAllocation);
}