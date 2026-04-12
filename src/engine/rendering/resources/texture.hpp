#pragma once

#include <string>

#include "rendering/utils/enum_flags.hpp"

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

namespace niji
{

struct Size3D
{
    uint32_t X = 0u;
    uint32_t Y = 0u;
    uint32_t Z = 0u;

    inline bool is_2d() const
    {
        return Z == 0u;
    }
};

enum class TextureFormat : uint32_t
{
    Invalid = 0u,
    RGBA8Unorm,
    RGBA16SFloat,
    D32SFloat
};

enum class TextureUsage : uint32_t
{
    Invalid = 0u,
    TransferDst = 1u << 1u,     // Can Be Written to by a Transfer Command
    TransferSrc = 1u << 2u,     // Can be Read from by a Transfer Command
    Sampled = 1u << 3u,         // Read-only Texture
    Storage = 1u << 4u,         // RW Texture
    ColorAttachment = 1u << 5u, // Raster Pass Color Attachment
    DepthStencil = 1u << 6u     // Raster Pass DepthStencil Attachment
};
ENUM_CLASS_FLAGS(TextureUsage);

struct TextureDesc
{
    std::string Name = "Unknown Texture";
    TextureFormat Format = TextureFormat::Invalid;
    TextureUsage Usage = TextureUsage::Invalid;

    uint32_t Mips = 1u;
    uint32_t Layers = 1u;

    Size3D Size {};
};

// Image View Wrapper
struct ImageView
{
    VkImageView View {};
    VkImageSubresourceRange SubRange {};
};

// Texture Resource
struct Texture
{
    Texture() = default;

    VmaAllocation Allocation {};
    VkImage Image {};
    ImageView FullView {}; // Used For All Images

    TextureDesc Desc {};
    std::vector<ImageView> MippedViews {}; // Used For Storage Images (we can write to individual mips of a Storage Image)

    VkImageLayout Layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

} // namespace niji