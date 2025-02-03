#pragma once

//#include <vector>

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

struct NijiBuffer
{
    VkBuffer Buffer = {};
    VmaAllocation BufferAllocation = {};
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