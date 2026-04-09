#include "buffer.hpp"

#include <stdexcept>
#include <vk_mem_alloc.h>

#include "engine.hpp"

namespace niji
{

Buffer::Buffer(BufferDesc& desc, void* data)
{
    Desc = desc;
    Data = data;
    VkBuffer stagingBuffer = {};
    VmaAllocation stagingBufferAllocation = {};

    VkBufferUsageFlags usageFlags = {};
    VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (!desc.IsPersistent/*desc.Usage != BufferDesc::BufferUsage::Uniform &&
        desc.Usage != BufferDesc::BufferUsage::Storage*/)
    {
        usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        nijiEngine.m_context.create_buffer(desc.Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer,
                                           stagingBufferAllocation);

        void* dataStaged = nullptr;
        vmaMapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation, &dataStaged);
        if (Data)
            memcpy(dataStaged, Data, (size_t)desc.Size);
        else
            memset(dataStaged, 0, (size_t)desc.Size);
        vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation);
    }

    switch (desc.Usage)
    {
    case BufferDesc::BufferUsage::Invalid:
        throw std::runtime_error("Failed to Create Buffer. Invalid Usage Set!");
        break;
    case BufferDesc::BufferUsage::Vertex:
        usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if (desc.IsPersistent)
            memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        break;
    case BufferDesc::BufferUsage::Index:
        usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;
    case BufferDesc::BufferUsage::Uniform:
        usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        break;
    case BufferDesc::BufferUsage::Storage:
        usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        break;
    default:
        break;
    }

    nijiEngine.m_context.create_buffer(desc.Size, usageFlags, memUsage, Handle, BufferAllocation,
                                       desc.IsPersistent);
    vmaSetAllocationName(nijiEngine.m_context.m_allocator, BufferAllocation, desc.Name);

    if (!desc.IsPersistent/*desc.Usage != BufferDesc::BufferUsage::Uniform &&
        desc.Usage != BufferDesc::BufferUsage::Storage*/)
    {
        nijiEngine.m_context.copy_buffer(stagingBuffer, Handle, desc.Size);
        vmaDestroyBuffer(nijiEngine.m_context.m_allocator, stagingBuffer, stagingBufferAllocation);
    }
    else if (desc.IsPersistent/*desc.Usage == BufferDesc::BufferUsage::Uniform ||
             desc.Usage == BufferDesc::BufferUsage::Storage*/)
    {
        vmaMapMemory(nijiEngine.m_context.m_allocator, BufferAllocation, &Data);
        Mapped = true;
    }

    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_BUFFER, Handle, desc.Name);
}

Buffer::~Buffer()
{
    cleanup();
}

Buffer::Buffer(Buffer&& other) noexcept
{
    *this = std::move(other);
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other)
    {
        cleanup();

        Handle = other.Handle;
        BufferAllocation = other.BufferAllocation;
        Desc = std::move(other.Desc);
        Data = other.Data;
        Mapped = other.Mapped;

        other.Handle = VK_NULL_HANDLE;
        other.BufferAllocation = nullptr;
        other.Data = nullptr;
        other.Mapped = false;
    }
    return *this;
}

void Buffer::cleanup()
{
    if (BufferAllocation != nullptr)
    {
        if (Mapped)
        {
            vmaUnmapMemory(nijiEngine.m_context.m_allocator, BufferAllocation);
            Mapped = false;
        }

        if (Handle != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(nijiEngine.m_context.m_allocator, Handle, BufferAllocation);
        }

        Handle = VK_NULL_HANDLE;
        BufferAllocation = nullptr;
        Data = nullptr;
    }
}

} // namespace niji