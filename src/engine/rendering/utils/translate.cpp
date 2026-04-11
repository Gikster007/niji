#include "translate.hpp"

#include <cassert>

namespace niji
{
namespace translate
{

VkFormat texture_format(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::RGBA8Unorm:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::RGBA16SFloat:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case TextureFormat::D32SFloat:
        return VK_FORMAT_D32_SFLOAT;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

VkImageAspectFlags aspect_from_format(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::D32SFloat:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

VkImageUsageFlags texture_usage(TextureUsage usage)
{
    VkImageUsageFlags flags = 0x00;
    if (has_flag(usage, TextureUsage::TransferDst))
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (has_flag(usage, TextureUsage::TransferSrc))
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (has_flag(usage, TextureUsage::Sampled))
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (has_flag(usage, TextureUsage::Storage))
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (has_flag(usage, TextureUsage::ColorAttachment))
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (has_flag(usage, TextureUsage::DepthStencil))
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    return flags;
}

VkFilter sampler_filter(SamplerDesc::Filter filter)
{
    switch (filter)
    {
    case niji::SamplerDesc::Filter::NONE:
        assert(!"Invalid Sampler Filter Mode!");
        break;
    case niji::SamplerDesc::Filter::NEAREST:
        return VK_FILTER_NEAREST;
        break;
    case niji::SamplerDesc::Filter::LINEAR:
        return VK_FILTER_LINEAR;
        break;
    }
}

VkSamplerAddressMode sampler_address_mode(SamplerDesc::AddressMode addressMode)
{
    switch (addressMode)
    {
    case niji::SamplerDesc::AddressMode::NONE:
        assert(!"Invalid Sampler Address Mode!");
        break;
    case niji::SamplerDesc::AddressMode::REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;
    case niji::SamplerDesc::AddressMode::MIRRORED_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        break;
    case niji::SamplerDesc::AddressMode::EDGE_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        break;
    case niji::SamplerDesc::AddressMode::BORDER_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        break;
    case niji::SamplerDesc::AddressMode::MIRRORED_EDGE_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        break;
    }
}

VkSamplerMipmapMode sampler_mipmap_mode(SamplerDesc::MipMapMode mipmapMode)
{
    switch (mipmapMode)
    {
    case niji::SamplerDesc::MipMapMode::NONE:
        assert(!"Invalid Sampler Mip Map Mode!");
        break;
    case niji::SamplerDesc::MipMapMode::NEAREST:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        break;
    case niji::SamplerDesc::MipMapMode::LINEAR:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        break;
    }
}

VkBufferUsageFlags buffer_usage(BufferUsage usage)
{
    VkBufferUsageFlags flags = 0x00;
    if (has_flag(usage, BufferUsage::TransferDst))
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (has_flag(usage, BufferUsage::TransferSrc))
        flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (has_flag(usage, BufferUsage::Vertex))
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (has_flag(usage, BufferUsage::Index))
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (has_flag(usage, BufferUsage::Uniform))
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (has_flag(usage, BufferUsage::Storage))
        flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    return flags;
}

} // namespace translate
} // namespace niji
