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
        vmaCreateAllocator(&allocatorInfo, &m_allocator);
    }

    // Initialize the Resource Pools
    m_textures.init(m_max_textures);
    m_samplers.init(m_max_samplers);
    m_buffers.init(m_max_buffers);
}

} // namespace niji