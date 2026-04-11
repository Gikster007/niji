#include "resource_bank.hpp"

#include <vk_mem_alloc.h>

#include "engine.hpp"
#include "core/context.hpp"

#include "utils/translate.hpp"

namespace niji
{

void ResourceBank::init()
{
    const Context& context = nijiEngine.m_context;

    // Initialize the VMA Memory Allocator
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = context.m_physicalDevice;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorInfo.device = context.m_device;
        allocatorInfo.instance = context.m_instance;

        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
            assert(!"[ResourceBank] Failed to Create VMA Allocator");
    }

    // Initialize the Resource Pools
    m_textures.init(m_max_textures);
    m_samplers.init(m_max_samplers);
    m_buffers.init(m_max_buffers);

    // Create Bindless Descriptor Set (Supports Sampled/Storage Images and Samplers)
    // For Buffers, We Use BufferDeviceAddress So No Binding is Necessary
    {
        VkDescriptorSetLayoutBinding bindings[3] {};

        // Binding 0: Sampled Images
        bindings[BINDLESS_SAMPLED_IMAGES_BINDING].binding = 0;
        bindings[BINDLESS_SAMPLED_IMAGES_BINDING].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[BINDLESS_SAMPLED_IMAGES_BINDING].descriptorCount = m_textures.capacity();
        bindings[BINDLESS_SAMPLED_IMAGES_BINDING].stageFlags = VK_SHADER_STAGE_ALL;

        // Binding 1: Storage Images
        bindings[BINDLESS_STORAGE_IMAGES_BINDING].binding = 1;
        bindings[BINDLESS_STORAGE_IMAGES_BINDING].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[BINDLESS_STORAGE_IMAGES_BINDING].descriptorCount = m_textures.capacity() * MAX_MIPS;
        bindings[BINDLESS_STORAGE_IMAGES_BINDING].stageFlags = VK_SHADER_STAGE_ALL;

        // Binding 2: Samplers
        bindings[BINDLESS_SAMPLERS_BINDING].binding = 2;
        bindings[BINDLESS_SAMPLERS_BINDING].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        bindings[BINDLESS_SAMPLERS_BINDING].descriptorCount = m_samplers.capacity();
        bindings[BINDLESS_SAMPLERS_BINDING].stageFlags = VK_SHADER_STAGE_ALL;

        // Binding Flags
        VkDescriptorBindingFlags bindingFlags[] = {
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        };

        // Create Descriptor Set Layout
        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo {};
        flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flagsInfo.bindingCount = static_cast<uint32_t>(sizeof(bindingFlags) / sizeof(VkDescriptorBindingFlags));
        flagsInfo.pBindingFlags = bindingFlags;

        VkDescriptorSetLayoutCreateInfo layoutInfo {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.pNext = &flagsInfo;
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        layoutInfo.bindingCount = static_cast<uint32_t>(sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding));
        layoutInfo.pBindings = bindings;

        if (vkCreateDescriptorSetLayout(context.m_device, &layoutInfo, nullptr, &m_bindless_set_layout) != VK_SUCCESS)
            assert(!"[ResourceBank] Failed to Create Bindless Descriptor Set Layout");

        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, m_bindless_set_layout,
                      "Bindless Descriptor Set Layout");

        // Create Descriptor Pool
        VkDescriptorPoolSize poolSizes[3] {};
        poolSizes[BINDLESS_SAMPLED_IMAGES_BINDING] = {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_textures.capacity()};
        poolSizes[BINDLESS_STORAGE_IMAGES_BINDING] = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_textures.capacity() * MAX_MIPS};
        poolSizes[BINDLESS_SAMPLERS_BINDING] = {VK_DESCRIPTOR_TYPE_SAMPLER, m_samplers.capacity()};

        VkDescriptorPoolCreateInfo poolInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = static_cast<uint32_t>(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
        poolInfo.pPoolSizes = poolSizes;

        if (vkCreateDescriptorPool(context.m_device, &poolInfo, nullptr, &m_bindless_pool) != VK_SUCCESS)
            assert(!"[ResourceBank] Failed to Create Bindless Descriptor Pool");

        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_POOL, m_bindless_pool, "Bindless Pool");

        // Create Descriptor Set
        VkDescriptorSetAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_bindless_pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_bindless_set_layout;

        if (vkAllocateDescriptorSets(context.m_device, &allocInfo, &m_bindless_set) != VK_SUCCESS)
            assert(!"[ResourceBank] Failed to Create Bindless Descriptor Set");

        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET, m_bindless_set, "Bindless Set");
    }
}

TextureHandle ResourceBank::create_texture(TextureDesc desc)
{
    if (desc.Format == TextureFormat::Invalid)
        assert(!"[ResourceBank] Tried Creating Texture With Invalid Texture Format");

    const VkFormat format = translate::texture_format(desc.Format);

    // Get New Resource and Handle
    PoolPair texture = m_textures.pop();
    texture.Data.Desc = desc; // Set Texture Info on Texture Resource

    // Image Creation Info
    VkImageCreateInfo textureInfo {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    textureInfo.imageType = desc.Size.is_2d() ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    textureInfo.format = format;
    textureInfo.extent = {std::max(desc.Size.X, 1u), std::max(desc.Size.Y, 1u), std::max(desc.Size.Z, 1u)};
    textureInfo.mipLevels = std::max(1u, desc.Mips);
    textureInfo.arrayLayers = std::max(1u, desc.Layers);
    textureInfo.samples = VK_SAMPLE_COUNT_1_BIT; // No MSAA
    textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    textureInfo.usage = translate::texture_usage(desc.Usage);
    textureInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Memory Allocation Info
    VmaAllocationCreateInfo allocInfo {};
    allocInfo.flags = 0x00u;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // Create The VkImage & Allocate it Using VMA
    if (vmaCreateImage(m_allocator, &textureInfo, &allocInfo, &texture.Data.Image, &texture.Data.Allocation, nullptr) !=
        VK_SUCCESS)
        assert(!"[ResourceBank] Failed to Create Image Using VMA");

    const Context& context = nijiEngine.m_context;

    SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_IMAGE, texture.Data.Image, desc.Name.c_str());

    // Create Full Image View
    ImageViewDesc viewDesc {};
    viewDesc.Format = desc.Format;
    texture.Data.FullView = create_image_view(texture.Data.Image, viewDesc);

    SetObjectName(context.m_device, VK_OBJECT_TYPE_IMAGE_VIEW, texture.Data.FullView, (desc.Name + " [Full View]").c_str());

    // Write Sampled Images to Bindless Descriptor Set
    if (has_flag(desc.Usage, TextureUsage::Sampled))
    {
        VkDescriptorImageInfo imageInfo {};
        imageInfo.imageView = texture.Data.FullView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet write {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = m_bindless_set;
        write.dstBinding = BINDLESS_SAMPLED_IMAGES_BINDING;
        write.dstArrayElement = texture.Handle.Index - 1u;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(context.m_device, 1, &write, 0, nullptr);
    }

    // Create Necessary Image Views For Storage Images
    // And Insert Accordingly in Bindless Descriptor Set
    if (has_flag(desc.Usage, TextureUsage::Storage))
    {
        texture.Data.MippedViews.resize(std::max(1u, desc.Mips));

        for (uint32_t mip = 0; mip < texture.Data.MippedViews.size(); mip++)
        {
            VkImageView& imageView = texture.Data.MippedViews[mip];

            viewDesc = {};
            viewDesc.Format = desc.Format;
            viewDesc.BaseMip = mip;
            viewDesc.Mips = 1;

            imageView = create_image_view(texture.Data.Image, viewDesc);

            SetObjectName(context.m_device, VK_OBJECT_TYPE_IMAGE_VIEW, imageView,
                          (desc.Name + " [Storage Mip " + std::to_string(mip) + "]").c_str());

            VkDescriptorImageInfo imageInfo {};
            imageInfo.imageView = imageView;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkWriteDescriptorSet write {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_bindless_set;
            write.dstBinding = BINDLESS_STORAGE_IMAGES_BINDING;
            write.dstArrayElement = (texture.Handle.Index - 1u) * MAX_MIPS + mip;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(context.m_device, 1, &write, 0, nullptr);
        }
    }
    return texture.Handle;
}

SamplerHandle ResourceBank::create_sampler(SamplerDesc desc)
{
    const Context& context = nijiEngine.m_context;

    // Get New Resource and Handle
    PoolPair sampler = m_samplers.pop();

    // Sampler Creation Info
    VkSamplerCreateInfo samplerInfo {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = translate::sampler_filter(desc.MagFilter);
    samplerInfo.minFilter = translate::sampler_filter(desc.MinFilter);
    samplerInfo.mipmapMode = translate::sampler_mipmap_mode(desc.MipmapMode);
    samplerInfo.addressModeU = translate::sampler_address_mode(desc.AddressModeU);
    samplerInfo.addressModeV = translate::sampler_address_mode(desc.AddressModeV);
    samplerInfo.addressModeW = translate::sampler_address_mode(desc.AddressModeW);
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    // Create The Sampler
    if (vkCreateSampler(context.m_device, &samplerInfo, nullptr, &sampler.Data.Object) != VK_SUCCESS)
        assert(!"[ResourceBank] Failed to Create Sampler");

    SetObjectName(context.m_device, VK_OBJECT_TYPE_SAMPLER, sampler.Data.Object, desc.Name.c_str());

    // Write Sampler to Bindless Descriptor Set
    VkDescriptorImageInfo imageInfo {};
    imageInfo.imageView = VK_NULL_HANDLE;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.sampler = sampler.Data.Object;

    VkWriteDescriptorSet write {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_bindless_set;
    write.dstBinding = BINDLESS_SAMPLERS_BINDING;
    write.dstArrayElement = sampler.Handle.Index - 1u;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(context.m_device, 1, &write, 0, nullptr);

    return sampler.Handle;
}

BufferHandle ResourceBank::create_buffer(BufferDesc desc)
{
    if (desc.Usage == BufferUsage::Invalid)
        assert(!"[ResourceBank] Invalid Buffer Usage");

    if (desc.Size < 1u)
        assert(!"[ResourceBank] Invalid Buffer Size");

    // Get New Resource and Handle
    PoolPair buffer = m_buffers.pop();
    buffer.Data.Desc = desc;

    // Buffer Creation Info
    VkBufferCreateInfo bufferInfo {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = desc.Size;
    bufferInfo.usage = translate::buffer_usage(desc.Usage);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Memory Allocation Info
    VmaAllocationCreateInfo allocInfo {};
    allocInfo.flags = 0x00u;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    /* Create the buffer & allocate it using VMA */
    if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &buffer.Data.Object, &buffer.Data.Allocation, nullptr) !=
        VK_SUCCESS)
        assert(!"[ResourceBank] Failed to Create Buffer Using VMA");

    const Context& context = nijiEngine.m_context;

    SetObjectName(context.m_device, VK_OBJECT_TYPE_BUFFER, buffer.Data.Object, desc.Name.c_str());

    // Store Address
    VkBufferDeviceAddressInfo addressInfo {};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = buffer.Data.Object;

    buffer.Data.Address = vkGetBufferDeviceAddress(context.m_device, &addressInfo);

    return buffer.Handle;
}

VkDeviceAddress ResourceBank::get_buffer_address(BufferHandle buffer) const
{
    return m_buffers.get(buffer).Address;
}

uint32_t ResourceBank::get_storage_tex_index(TextureHandle texture, uint32_t mip) const
{
    return texture.Index * MAX_MIPS + mip;
}

VkImageView ResourceBank::create_image_view(VkImage image, ImageViewDesc desc)
{
    const Context& context = nijiEngine.m_context;

    const VkFormat format = translate::texture_format(desc.Format);

    VkImageViewCreateInfo viewInfo {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = translate::aspect_from_format(desc.Format);
    viewInfo.subresourceRange.baseMipLevel = desc.BaseMip;
    viewInfo.subresourceRange.levelCount = desc.Mips;
    viewInfo.subresourceRange.baseArrayLayer = desc.BaseLayer;
    viewInfo.subresourceRange.layerCount = desc.Layers;

    VkImageView view = VK_NULL_HANDLE;
    if (vkCreateImageView(context.m_device, &viewInfo, nullptr, &view) != VK_SUCCESS)
        assert(!"[ResourceBank] Failed to Create Image View");

    return view;
}

} // namespace niji