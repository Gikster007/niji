#pragma once

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

namespace niji
{
void SetObjectName(VkDevice device, VkBuffer buffer, const char* name);

struct BufferDesc
{
    VkDeviceSize Size = {};
    enum class BufferUsage
    {
        Invalid,
        Vertex,
        Index,
    } Usage = {};
    bool IsPersistent = false;
    char* Name = "Unknown Buffer";
};

struct Buffer
{
    Buffer() = default;
    Buffer(BufferDesc& desc, void* data);

    VkBuffer Handle = {};
    VmaAllocation BufferAllocation = {};
    BufferDesc Desc = {};
};

struct NijiTexture
{
    VkImage TextureImage = {};
    VkImageView TextureImageView = {};
    VmaAllocation TextureImageAllocation = {};
};

struct NijiUBO
{
    std::vector<VkBuffer> UniformBuffers = {};
    std::vector<VmaAllocation> UniformBuffersAllocations = {};
    std::vector<void*> UniformBuffersMapped = {};
};
} // namespace niji
