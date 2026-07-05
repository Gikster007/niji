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

constexpr uint32_t MAX_MIPS = 13u; // Supports Up To 4096x4096
constexpr uint32_t MAX_SWAPCHAIN_IMAGES = 8u;

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
    ResourceBank() = default;

    void init();
    void deinit();

    inline void set_max_textures(const uint32_t count)
    {
        m_maxTextures = count;
    }

    inline void set_max_samplers(const uint32_t count)
    {
        m_maxSamplers = count;
    }

    inline void set_max_buffers(const uint32_t count)
    {
        m_maxBuffers = count;
    }

    // Create Render Target
    RenderTargetHandle create_render_target(uint32_t width, uint32_t height);
    // Create Texture Resource
    TextureHandle create_texture(TextureDesc desc);
    // Create Sampler Resource
    SamplerHandle create_sampler(SamplerDesc desc);
    // Create Buffer Resource
    BufferHandle create_buffer(BufferDesc desc);

    // Upload to a Texture Resource
    void upload_texture(TextureHandle handle, const void* data, uint64_t size);
    // Upload to a Buffer Resource
    void upload_buffer(BufferHandle handle, const void* data, uint64_t dstOffset, uint64_t size);

    // Returns the Memory Address of the Given Buffer
    // Can be Directly Accessed on the GPU (hint: pass via Push Constants)
    uint64_t get_buffer_address(BufferHandle buffer) const;

    // Returns the Bindless Index of the Given Texture
    // Used for passing to the Shader (supports individual storage mips)
    uint32_t get_storage_tex_index(TextureHandle texture, uint32_t mip = 0u) const;

    void destroy(ResourceHandle& handle);

  private:
    // Creates the VkImageView and Subresource Range given a VkImage and a ImageViewDesc
    void create_image_view(ImageView& imageView, VkImage image, ImageViewDesc desc);

    // Generates Mip Chain for the Given Texture
    void generate_mips(TextureHandle handle);

    // Prepare Upload Command Buffer for Uploading
    bool begin_upload_cmd() const;
    // End Upload Command Buffer, Submit it and Wait on the Upload Fence
    bool end_upload_cmd() const;

    // Cleans up and Destroys a Render Target
    void destroy_render_target(RenderTargetHandle& handle);
    // Cleans up and Destroys a Texture
    void destroy_texture(TextureHandle& handle);
    // Cleans up and Destroys a Sampler
    void destroy_sampler(SamplerHandle& handle);
    // Cleans up and Destroys a Buffer
    void destroy_buffer(BufferHandle& handle);

    // TODO: Make this private (so that they will only be accessed from the Render Graph)
  public:
    // VMA Allocator
    VmaAllocator m_allocator = {};

    // Bindless Descriptor
    VkDescriptorSetLayout m_bindlessSetLayout {};
    VkDescriptorPool m_bindlessPool {};
    VkDescriptorSet m_bindlessSet {};

    // Upload
    VkCommandPool m_uploadCmdPool {};
    VkCommandBuffer m_uploadCmd {};
    VkFence m_uploadFence {};

    // Resource Pools
    Pool<RenderTarget, RenderTargetHandle, ResourceType::RenderTarget> m_renderTargets {};
    Pool<Texture, TextureHandle, ResourceType::Texture> m_textures {};
    Pool<Sampler, SamplerHandle, ResourceType::Sampler> m_samplers {};
    Pool<Buffer, BufferHandle, ResourceType::Buffer> m_buffers {};

    uint32_t m_maxRenderTargets = 2u;
    uint32_t m_maxTextures = 8u;
    uint32_t m_maxSamplers = 8u;
    uint32_t m_maxBuffers = 8u;
};

} // namespace niji