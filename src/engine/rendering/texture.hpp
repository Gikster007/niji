#pragma once

#include <string>

enum VmaMemoryUsage;
struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

namespace niji
{

struct RenderTarget;

struct TextureDesc
{
    int Width = 0;
    int Height = 0;
    int Channels = 4;
    unsigned char* Data = nullptr;
    char* Name = nullptr;

    enum class TextureType
    {
        NONE,
        TEXTURE_2D,
        CUBEMAP
    } Type = TextureType::NONE;

    bool IsReadWrite = false;
    bool IsMipMapped = false;
    bool ShowInImGui = false;
    uint32_t Mips = 1;
    uint32_t Layers = 1;

    VkFormat Format = {};
    VkImageUsageFlags Usage = {};
    VmaMemoryUsage MemoryUsage = {};
};

struct Texture
{
    Texture() = default;
    Texture(const TextureDesc& desc);
    // Used only for Envmap Loading
    Texture(const TextureDesc& desc, const std::string& path);
    // Used only for translating a Depth RT
    Texture(RenderTarget& depthRT);

    void cleanup() const;

    TextureDesc Desc = {};

    VkImage TextureImage = {};
    VkImageView TextureImageView = {};
    VmaAllocation TextureImageAllocation = {};

    VkDescriptorImageInfo ImageInfo = {};

    VkDescriptorSet ImGuiHandle = {};
};

} // namespace niji