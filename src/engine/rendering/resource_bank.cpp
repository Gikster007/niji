#include "resource_bank.hpp"

#include <vk_mem_alloc.h>

#include "engine.hpp"
#include "core/context.hpp"

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
        {
            assert("[ResourceBank] Failed to Create VMA Allocator");
        }
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
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[0].descriptorCount = m_textures.capacity();
        bindings[0].stageFlags = VK_SHADER_STAGE_ALL;

        // Binding 1: Storage Images
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[1].descriptorCount = m_textures.capacity();
        bindings[1].stageFlags = VK_SHADER_STAGE_ALL;

        // Binding 2: Samplers
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        bindings[2].descriptorCount = m_samplers.capacity();
        bindings[2].stageFlags = VK_SHADER_STAGE_ALL;

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
        {
            assert("[ResourceBank] Failed to Create Bindless Descriptor Set Layout");
        }
        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, m_bindless_set_layout, "Bindless Descriptor Set Layout");

        // Create Descriptor Pool
        VkDescriptorPoolSize poolSizes[3] {};
        poolSizes[0] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_textures.capacity() };
        poolSizes[1] = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_textures.capacity() };
        poolSizes[2] = { VK_DESCRIPTOR_TYPE_SAMPLER,       m_samplers.capacity() };

        VkDescriptorPoolCreateInfo poolInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = static_cast<uint32_t>(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
        poolInfo.pPoolSizes = poolSizes;

        if (vkCreateDescriptorPool(context.m_device, &poolInfo, nullptr, &m_bindless_pool) != VK_SUCCESS)
        {
            assert("[ResourceBank] Failed to Create Bindless Descriptor Pool");
        }
        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_POOL, m_bindless_pool, "Bindless Pool");

        // Create Descriptor Set
        VkDescriptorSetAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_bindless_pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_bindless_set_layout;

        if (vkAllocateDescriptorSets(context.m_device, &allocInfo, &m_bindless_set) != VK_SUCCESS)
        {
            assert("[ResourceBank] Failed to Create Bindless Descriptor Set");
        }
        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET, m_bindless_set, "Bindless Set");
    }
}

} // namespace niji