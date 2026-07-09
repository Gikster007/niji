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
    case TextureFormat::RGBA8Srgb:
        return VK_FORMAT_R8G8B8A8_SRGB;
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
        return VK_FILTER_MAX_ENUM;
    case niji::SamplerDesc::Filter::NEAREST:
        return VK_FILTER_NEAREST;
    case niji::SamplerDesc::Filter::LINEAR:
        return VK_FILTER_LINEAR;
    }
    return VK_FILTER_MAX_ENUM;
}

VkSamplerAddressMode sampler_address_mode(SamplerDesc::AddressMode addressMode)
{
    switch (addressMode)
    {
    case niji::SamplerDesc::AddressMode::NONE:
        assert(!"Invalid Sampler Address Mode!");
        return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
    case niji::SamplerDesc::AddressMode::REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case niji::SamplerDesc::AddressMode::MIRRORED_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case niji::SamplerDesc::AddressMode::EDGE_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case niji::SamplerDesc::AddressMode::BORDER_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case niji::SamplerDesc::AddressMode::MIRRORED_EDGE_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    }
    return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
}

VkSamplerMipmapMode sampler_mipmap_mode(SamplerDesc::MipMapMode mipmapMode)
{
    switch (mipmapMode)
    {
    case niji::SamplerDesc::MipMapMode::NONE:
        assert(!"Invalid Sampler Mip Map Mode!");
        return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
    case niji::SamplerDesc::MipMapMode::NEAREST:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case niji::SamplerDesc::MipMapMode::LINEAR:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
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

VkPipelineStageFlags2 to_vk_stages(DependencyStages stages)
{
    VkPipelineStageFlags2 result = VK_PIPELINE_STAGE_2_NONE;
    if (has_flag(stages, DependencyStages::Compute))
        result |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    if (has_flag(stages, DependencyStages::Vertex))
        result |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    if (has_flag(stages, DependencyStages::Pixel))
        result |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    return result;
}

VkShaderStageFlags to_vk_shader_stages(DependencyStages stages)
{
    VkShaderStageFlags result = 0;
    if (has_flag(stages, DependencyStages::Compute))
        result |= VK_SHADER_STAGE_COMPUTE_BIT;
    if (has_flag(stages, DependencyStages::Vertex))
        result |= VK_SHADER_STAGE_VERTEX_BIT;
    if (has_flag(stages, DependencyStages::Pixel))
        result |= VK_SHADER_STAGE_FRAGMENT_BIT;
    return result;
}

VkAccessFlags2 to_vk_access(DependencyUsage usage)
{
    switch (usage)
    {
    case DependencyUsage::IndexBuffer:
        return VK_ACCESS_2_INDEX_READ_BIT;
    case DependencyUsage::IndirectBuffer:
        return VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    case DependencyUsage::Readonly:
        return VK_ACCESS_2_SHADER_READ_BIT;
    case DependencyUsage::ReadWrite:
        return VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    case DependencyUsage::ColorAttachment:
        return VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    case DependencyUsage::DepthStencil:
        return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    default:
        return VK_ACCESS_2_NONE;
    }
}

VkPrimitiveTopology primitive_topology(const Topology topology)
{
    switch (topology)
    {
    case Topology::TriangleList:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case Topology::LineList:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    default:
        return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }
}

bool is_stencil_format(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::D24UnormS8Uint:
    case TextureFormat::D32SFloatS8Uint:
        return true;
    default:
        return false;
    }
}

VkStencilOp stencil_op(StencilOp op)
{
    switch (op)
    {
    case StencilOp::Keep:
        return VK_STENCIL_OP_KEEP;
    case StencilOp::Zero:
        return VK_STENCIL_OP_ZERO;
    case StencilOp::Replace:
        return VK_STENCIL_OP_REPLACE;
    case StencilOp::IncrementClamp:
        return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case StencilOp::DecrementClamp:
        return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case StencilOp::Invert:
        return VK_STENCIL_OP_INVERT;
    case StencilOp::IncrementWrap:
        return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case StencilOp::DecrementWrap:
        return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    default:
        return VK_STENCIL_OP_MAX_ENUM;
    }
}

VkCompareOp compare_op(CompareOp op)
{
    switch (op)
    {
    case CompareOp::Never:
        return VK_COMPARE_OP_NEVER;
    case CompareOp::Less:
        return VK_COMPARE_OP_LESS;
    case CompareOp::Equal:
        return VK_COMPARE_OP_EQUAL;
    case CompareOp::LessEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOp::Greater:
        return VK_COMPARE_OP_GREATER;
    case CompareOp::NotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOp::GreaterEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOp::Always:
        return VK_COMPARE_OP_ALWAYS;
    default:
        return VK_COMPARE_OP_MAX_ENUM;
    }
}

VkStencilOpState stencil_op_state(StencilState state)
{
    // Configure what happens on stencil / depth pass / fail
    VkStencilOpState stencil_op_state {};
    stencil_op_state.failOp = stencil_op(state.FailOp);           // operation on stencil test fail
    stencil_op_state.passOp = stencil_op(state.PassOp);           // operation on stencil test pass
    stencil_op_state.depthFailOp = stencil_op(state.DepthFailOp); // operation on depth test fail
    stencil_op_state.compareOp = compare_op(state.Compare);       // comparison operation
    stencil_op_state.compareMask = state.CompareMask;             // comparison mask
    stencil_op_state.writeMask = state.WriteMask;                 // write mask
    stencil_op_state.reference = state.ReferenceValue;            // value to write/compare against

    return stencil_op_state;
}

VkAttachmentLoadOp load_operation(const LoadOp op)
{
    switch (op)
    {
    case LoadOp::Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case LoadOp::Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    default:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

VkShaderStageFlags stage_flags(DependencyStages stages)
{
    VkShaderStageFlags flags = 0x00;
    if (has_flag(stages, DependencyStages::Compute))
        flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    if (has_flag(stages, DependencyStages::Vertex))
        flags |= VK_SHADER_STAGE_VERTEX_BIT;
    if (has_flag(stages, DependencyStages::Pixel))
        flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    return flags;
}

} // namespace translate
} // namespace niji
