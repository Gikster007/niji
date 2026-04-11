#include "sampler.hpp"

#include <stdexcept>

#include "engine.hpp"

namespace niji
{

//inline static VkFilter to_vk(SamplerDesc::Filter filterMode)
//{
//    switch (filterMode)
//    {
//    case niji::SamplerDesc::Filter::NONE:
//        throw std::runtime_error("Invalid Sampler Filter Mode!");
//        break;
//    case niji::SamplerDesc::Filter::NEAREST:
//        return VK_FILTER_NEAREST;
//        break;
//    case niji::SamplerDesc::Filter::LINEAR:
//        return VK_FILTER_LINEAR;
//        break;
//    default:
//        throw std::runtime_error("Invalid Sampler Filter Mode!");
//        break;
//    }
//}
//
//inline static VkSamplerAddressMode to_vk(SamplerDesc::AddressMode addressMode)
//{
//    switch (addressMode)
//    {
//    case niji::SamplerDesc::AddressMode::NONE:
//        throw std::runtime_error("Invalid Sampler Address Mode!");
//        break;
//    case niji::SamplerDesc::AddressMode::REPEAT:
//        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
//        break;
//    case niji::SamplerDesc::AddressMode::MIRRORED_REPEAT:
//        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
//        break;
//    case niji::SamplerDesc::AddressMode::EDGE_CLAMP:
//        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//        break;
//    case niji::SamplerDesc::AddressMode::BORDER_CLAMP:
//        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
//        break;
//    case niji::SamplerDesc::AddressMode::MIRRORED_EDGE_CLAMP:
//        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
//        break;
//    default:
//        throw std::runtime_error("Invalid Sampler Address Mode!");
//        break;
//    }
//}
//
//inline static VkSamplerMipmapMode to_vk(SamplerDesc::MipMapMode mipmapMode)
//{
//    switch (mipmapMode)
//    {
//    case niji::SamplerDesc::MipMapMode::NONE:
//        throw std::runtime_error("Invalid Sampler Mip Map Mode!");
//        break;
//    case niji::SamplerDesc::MipMapMode::NEAREST:
//        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
//        break;
//    case niji::SamplerDesc::MipMapMode::LINEAR:
//        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
//        break;
//    default:
//        throw std::runtime_error("Invalid Sampler Mip Map Mode!");
//        break;
//    }
//}

Sampler::Sampler(const SamplerDesc& desc)
{
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(nijiEngine.m_context.m_physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = to_vk(desc.MagFilter);
    samplerInfo.minFilter = to_vk(desc.MinFilter);
    samplerInfo.addressModeU = to_vk(desc.AddressModeU);
    samplerInfo.addressModeV = to_vk(desc.AddressModeV);
    samplerInfo.addressModeW = to_vk(desc.AddressModeW);
    samplerInfo.anisotropyEnable = desc.EnableAnisotropy;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    // Default to these values for now
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = to_vk(desc.MipmapMode);
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(desc.MaxMips);

    if (vkCreateSampler(nijiEngine.m_context.m_device, &samplerInfo, nullptr, &Handle) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Sampler!");

    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_SAMPLER, Handle, Desc.Name);
}

void Sampler::cleanup() const
{
    if (Handle != VK_NULL_HANDLE)
        vkDestroySampler(nijiEngine.m_context.m_device, Handle, nullptr);
}

} // namespace niji