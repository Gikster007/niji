#pragma once

#include "rendering/resources/texture.hpp"
#include "rendering/resources/sampler.hpp"
#include "rendering/resources/buffer.hpp"

namespace niji
{
namespace translate
{

// Convert From Agnostic Format to Vulkan Format
VkFormat texture_format(TextureFormat format);

// Convert From Agnostic Format to Vulkan Aspect Flags
VkImageAspectFlags aspect_from_format(TextureFormat format);

// Convert From Agnostic Usage to Vulkan Usage Flags
VkImageUsageFlags texture_usage(TextureUsage usage);

// Convert From Agnostic Filter to Vulkan Filter
VkFilter sampler_filter(SamplerDesc::Filter filter);

// Convert From Agnostic Address Mode to Vulkan Address Mode
VkSamplerAddressMode sampler_address_mode(SamplerDesc::AddressMode addressMode);

// Convert From Agnostic Mipmap Mode to Vulkan Mipmap Mode
VkSamplerMipmapMode sampler_mipmap_mode(SamplerDesc::MipMapMode mipmapMode);

// Convert From Agnostic Buffer Usage to Vulkan Buffer Usage
VkBufferUsageFlags buffer_usage(BufferUsage usage);

}
} // namespace niji