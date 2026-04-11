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

  private:
    // VMA Allocator
    VmaAllocator m_allocator = {};

    // Bindless Descriptor
    VkDescriptorSetLayout m_bindless_set_layout {};
    VkDescriptorPool m_bindless_pool {};
    VkDescriptorSet m_bindless_set {};

    Pool<Texture, TextureHandle, ResourceType::Texture> m_textures {};
    Pool<Sampler, SamplerHandle, ResourceType::Sampler> m_samplers {};
    Pool<Buffer, BufferHandle, ResourceType::Buffer> m_buffers {};

    uint32_t m_max_textures = 8u;
    uint32_t m_max_samplers = 8u;
    uint32_t m_max_buffers = 8u;
};

} // namespace niji