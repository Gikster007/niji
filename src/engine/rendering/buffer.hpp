#pragma once

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

namespace niji
{

struct BufferDesc
{
    VkDeviceSize Size = {};

    enum class BufferUsage
    {
        Invalid,
        Vertex,
        Index,
        Uniform,
        Storage
    } Usage = {};

    bool IsPersistent = false;
    char* Name = "Unknown Buffer";
};

struct Buffer
{
    Buffer() = default;
    Buffer(BufferDesc& desc, void* data);
    ~Buffer();

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    void cleanup();

    VkBuffer Handle = {};
    VmaAllocation BufferAllocation = {};
    BufferDesc Desc = {};

    void* Data = nullptr;
    bool Mapped = false;
};

} // namespace niji