#pragma once

#include "resources/resource_handle.hpp"
#include "resources/render_target.hpp"
#include "resources/sampler.hpp"
#include "resources/texture.hpp"
#include "resources/buffer.hpp"
#include "resources/pool.hpp"

struct VmaAllocator_T;
typedef VmaAllocator_T* VmaAllocator;

namespace niji
{

constexpr uint32_t BINDLESS_SAMPLED_IMAGES_BINDING = 0u;
constexpr uint32_t BINDLESS_STORAGE_IMAGES_BINDING = 1u;
constexpr uint32_t BINDLESS_SAMPLERS_BINDING = 2u;

constexpr uint32_t MAX_MIPS = 13; // Supports Up To 4096x4096

struct ImageViewDesc
{
    TextureFormat Format = TextureFormat::Invalid;
    uint32_t BaseMip = 0u;
    uint32_t Mips = VK_REMAINING_MIP_LEVELS;
    uint32_t BaseLayer = 0u;
    uint32_t Layers = VK_REMAINING_ARRAY_LAYERS;
};

class ResourceBank
{
  public:
    void init();

    inline void set_max_textures(const uint32_t count)
    {
        m_max_textures = count;
    }

    inline void set_max_samplers(const uint32_t count)
    {
        m_max_samplers = count;
    }

    inline void set_max_buffers(const uint32_t count)
    {
        m_max_buffers = count;
    }

    // Create Texture Resource
    TextureHandle create_texture(TextureDesc desc);
    // Create Sampler Resource
    SamplerHandle create_sampler(SamplerDesc desc);
    // Create Buffer Resource
    BufferHandle create_buffer(BufferDesc desc);

    // Returns the Memory Address of the Given Buffer
    // Can be Directly Accessed on the GPU (hint: pass via Push Constants)
    VkDeviceAddress get_buffer_address(BufferHandle buffer) const;

    // Returns the Bindless Index of the Given Texture
    // Used for passing to the Shader (supports individual storage mips)
    uint32_t get_storage_tex_index(TextureHandle texture, uint32_t mip = 0u) const;

  private:
    VkImageView create_image_view(VkImage image, ImageViewDesc desc);

  private:
    // VMA Allocator
    VmaAllocator m_allocator = {};

    // Bindless Descriptor
    VkDescriptorSetLayout m_bindless_set_layout {};
    VkDescriptorPool m_bindless_pool {};
    VkDescriptorSet m_bindless_set {};

    // Resource Pools
    Pool<Texture, TextureHandle, ResourceType::Texture> m_textures {};
    Pool<Sampler, SamplerHandle, ResourceType::Sampler> m_samplers {};
    Pool<Buffer, BufferHandle, ResourceType::Buffer> m_buffers {};

    uint32_t m_max_textures = 8u;
    uint32_t m_max_samplers = 8u;
    uint32_t m_max_buffers = 8u;
};

} // namespace niji