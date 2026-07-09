#pragma once

#include "rendering/resources/texture.hpp"
#include "rendering/resources/sampler.hpp"
#include "rendering/resources/buffer.hpp"

#include "rendering/rendergraph/nodes/node.hpp"
#include "rendering/rendergraph/nodes/raster_node.hpp"

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

// Convert From Agnostic Dependecy Stages to Vulkan Pipeline Stages
VkPipelineStageFlags2 to_vk_stages(DependencyStages stages);

// Convert From Agnostic Dependecy Stages to Vulkan Shader Stages
VkShaderStageFlags to_vk_shader_stages(DependencyStages stages);

// Convert From Agnostic Dependency Stages to Vulkan Access Flags
VkAccessFlags2 to_vk_access(DependencyUsage usage);

// Convert From Agnostic Primitive Topology to Vulkan Primitive Topology
VkPrimitiveTopology primitive_topology(const Topology topology);

// Check Agnostic Format for Stencil Format
bool is_stencil_format(TextureFormat format);

// Convert From Agnostic Stencil Op to Vulkan Stencil Op
VkStencilOp stencil_op(StencilOp op);

// Convert From Agnostic Compare Op to Vulkan Compare Op
VkCompareOp compare_op(CompareOp op);

// Convert From Agnostic Stencil State to Vulkan Stencil Op State
VkStencilOpState stencil_op_state(StencilState state);

// Convert From Agnostic Load Op to Vulkan Attachment Load Op
VkAttachmentLoadOp load_operation(const LoadOp op);

// Convert From Agnostic Dependency Stages to Vulkan Shader Stages
VkShaderStageFlags stage_flags(DependencyStages stages);

}
} // namespace niji