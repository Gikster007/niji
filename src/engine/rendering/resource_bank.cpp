#include "resource_bank.hpp"

#include <algorithm>
#include <stdexcept>

#include <vk_mem_alloc.h>
#include <imgui_impl_vulkan.h>

#include "engine.hpp"
#include "core/context.hpp"
#include "renderer.hpp"
#include "core/vulkan-functions.hpp"

#include "utils/translate.hpp"

namespace niji
{

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR Capabilities = {};
    std::vector<VkSurfaceFormatKHR> Formats = {};
    std::vector<VkPresentModeKHR> PresentModes = {};

    static SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface);
};

SwapchainSupportDetails SwapchainSupportDetails::query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapchainSupportDetails details = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
    }

    return details;
}

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
    m_renderTargets.init(m_maxRenderTargets);
    m_textures.init(m_maxTextures);
    m_samplers.init(m_maxSamplers);
    m_buffers.init(m_maxBuffers);

    // Create Bindless Descriptor Set (Supports Sampled/Storage Images and Samplers)
    // For Buffers, We Use BufferDeviceAddress So No Binding is Necessary
    {
        VkDescriptorSetLayoutBinding bindings[3] {};

        // Binding 0: Sampled Images
        bindings[BINDLESS_SAMPLED_IMAGES_BINDING].binding = 0;
        bindings[BINDLESS_SAMPLED_IMAGES_BINDING].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[BINDLESS_SAMPLED_IMAGES_BINDING].descriptorCount = m_textures.capacity() + MAX_SWAPCHAIN_IMAGES;
        bindings[BINDLESS_SAMPLED_IMAGES_BINDING].stageFlags = VK_SHADER_STAGE_ALL;

        // Binding 1: Storage Images
        bindings[BINDLESS_STORAGE_IMAGES_BINDING].binding = 1;
        bindings[BINDLESS_STORAGE_IMAGES_BINDING].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[BINDLESS_STORAGE_IMAGES_BINDING].descriptorCount = m_textures.capacity() * MAX_MIPS + MAX_SWAPCHAIN_IMAGES;
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

        if (vkCreateDescriptorSetLayout(context.m_device, &layoutInfo, nullptr, &m_bindlessSetLayout) != VK_SUCCESS)
            assert(!"[ResourceBank] Failed to Create Bindless Descriptor Set Layout");

        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, m_bindlessSetLayout, "Bindless Descriptor Set Layout");

        // Create Descriptor Pool
        VkDescriptorPoolSize poolSizes[3] {};
        poolSizes[BINDLESS_SAMPLED_IMAGES_BINDING] = {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_textures.capacity() + MAX_SWAPCHAIN_IMAGES};
        poolSizes[BINDLESS_STORAGE_IMAGES_BINDING] = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_textures.capacity() * MAX_MIPS + MAX_SWAPCHAIN_IMAGES};
        poolSizes[BINDLESS_SAMPLERS_BINDING] = {VK_DESCRIPTOR_TYPE_SAMPLER, m_samplers.capacity()};

        VkDescriptorPoolCreateInfo poolInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = static_cast<uint32_t>(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
        poolInfo.pPoolSizes = poolSizes;

        if (vkCreateDescriptorPool(context.m_device, &poolInfo, nullptr, &m_bindlessPool) != VK_SUCCESS)
            assert(!"[ResourceBank] Failed to Create Bindless Descriptor Pool");

        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_POOL, m_bindlessPool, "Bindless Pool");

        // Create Descriptor Set
        VkDescriptorSetAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_bindlessPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_bindlessSetLayout;

        if (vkAllocateDescriptorSets(context.m_device, &allocInfo, &m_bindlessSet) != VK_SUCCESS)
            assert(!"[ResourceBank] Failed to Create Bindless Descriptor Set");

        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET, m_bindlessSet, "Bindless Set");
    }

    // Create Upload Pool, Commandbuffer and Fence
    {
        // Command Pool Creation Info
        VkCommandPoolCreateInfo poolInfo {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = context.m_queueIndices.TransferFamily.value();

        // Create Ccommand Pool for Upload Command Buffer
        if (vkCreateCommandPool(context.m_device, &poolInfo, nullptr, &m_uploadCmdPool) != VK_SUCCESS)
            assert(!"[Resource Bank] Failed to Create Upload Command Pool");

        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_COMMAND_POOL, m_uploadCmdPool, "Upload Command Pool");

        // Allocate Upload Command Buffer
        VkCommandBufferAllocateInfo cmdAllocInfo {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cmdAllocInfo.commandPool = m_uploadCmdPool;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount = 1u;

        if (vkAllocateCommandBuffers(context.m_device, &cmdAllocInfo, &m_uploadCmd) != VK_SUCCESS)
            assert(!"[Resource Bank] Failed to Create Upload Command Buffer");

        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_COMMAND_BUFFER, m_uploadCmd, "Upload Command Buffer");

        // Create the Upload Fence
        VkFenceCreateInfo fenceInfo {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

        if (vkCreateFence(context.m_device, &fenceInfo, nullptr, &m_uploadFence) != VK_SUCCESS)
            assert(!"[Resource Bank] Failed to Create Upload Fence");

        SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_FENCE, m_uploadFence, "Upload Fence");
    }
}

void ResourceBank::deinit()
{
    const Context& context = nijiEngine.m_context;

    vkDestroyDescriptorPool(context.m_device, m_bindlessPool, nullptr);
    vkDestroyDescriptorSetLayout(context.m_device, m_bindlessSetLayout, nullptr);

    vkDestroyCommandPool(context.m_device, m_uploadCmdPool, nullptr);

    vkDestroyFence(context.m_device, m_uploadFence, nullptr);

    m_renderTargets.destroy();
    m_textures.destroy();
    m_samplers.destroy();
    m_buffers.destroy();

    vmaDestroyAllocator(m_allocator);
}

RenderTargetHandle ResourceBank::create_render_target(uint32_t width, uint32_t height)
{
     PoolPair renderTarget = m_renderTargets.pop();

    // Get Swapchain Info
    const SwapchainSupportDetails swapchainSupport = SwapchainSupportDetails::query_swapchain_support(nijiEngine.m_context.m_physicalDevice, nijiEngine.m_context.m_surface);

    const VkImageUsageFlags desiredUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

    if ((swapchainSupport.Capabilities.supportedUsageFlags & desiredUsage) != desiredUsage)
        assert(!"[Swapchain] Surface does not support required image usage (storage/sampled)");

    // Set Image Count
    renderTarget.Data.ImageCount = swapchainSupport.Capabilities.minImageCount;
    if (swapchainSupport.Capabilities.maxImageCount > 0 && renderTarget.Data.ImageCount > swapchainSupport.Capabilities.maxImageCount)
        renderTarget.Data.ImageCount = swapchainSupport.Capabilities.maxImageCount;
    uint32_t& imageCount = renderTarget.Data.ImageCount;

    // Set Extent
    if (swapchainSupport.Capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
    {
        renderTarget.Data.Extent = swapchainSupport.Capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, swapchainSupport.Capabilities.minImageExtent.width, swapchainSupport.Capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, swapchainSupport.Capabilities.minImageExtent.height, swapchainSupport.Capabilities.maxImageExtent.height);
        renderTarget.Data.Extent = actualExtent;
    }

    // Set Present Mode (Prefer VK_PRESENT_MODE_IMMEDIATE_KHR)
    for (const auto& availablePresentMode : swapchainSupport.PresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            renderTarget.Data.PresentMode = availablePresentMode;
            break;
        }
        else
            renderTarget.Data.PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    }

    const Context& context = nijiEngine.m_context;

    // Get Available Surface Formats
    uint32_t formatCount = 0u;
    vkGetPhysicalDeviceSurfaceFormatsKHR(context.m_physicalDevice, context.m_surface, &formatCount, nullptr);
    VkSurfaceFormatKHR* formats = new VkSurfaceFormatKHR[formatCount] {};
    vkGetPhysicalDeviceSurfaceFormatsKHR(context.m_physicalDevice, context.m_surface, &formatCount, formats);

    // Find an RGBA8 UNORM Format
    for (uint32_t i = 0u; i < formatCount; ++i)
    {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM)
        {
            renderTarget.Data.SurfaceFormat = formats[i].format;
            renderTarget.Data.ColorSpace = formats[i].colorSpace;
            break;
        }
    }
    delete[] formats; // Free the Formats

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = nijiEngine.m_context.m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = renderTarget.Data.SurfaceFormat;
    createInfo.imageColorSpace = renderTarget.Data.ColorSpace;
    createInfo.imageExtent = renderTarget.Data.Extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

    const QueueFamilyIndices indices = QueueFamilyIndices::find_queue_families(nijiEngine.m_context.m_physicalDevice, nijiEngine.m_context.m_surface);
    const uint32_t queueFamilyIndices[] = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

    if (indices.GraphicsFamily != indices.PresentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = renderTarget.Data.PresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(nijiEngine.m_context.m_device, &createInfo, nullptr, &renderTarget.Data.Handle) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Swap Chain!");

    vkGetSwapchainImagesKHR(nijiEngine.m_context.m_device, renderTarget.Data.Handle, &imageCount, nullptr);
    renderTarget.Data.Images.resize(imageCount);
    vkGetSwapchainImagesKHR(nijiEngine.m_context.m_device, renderTarget.Data.Handle, &imageCount, renderTarget.Data.Images.data());

    // Create Image Views
    const VkSemaphoreCreateInfo semaphoreInfo {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    renderTarget.Data.ImageViews.resize(imageCount);
    renderTarget.Data.Layouts.resize(imageCount);
    renderTarget.Data.Semaphores.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++)
    {
        // Image View Creation Info
        VkImageViewCreateInfo viewInfo {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = renderTarget.Data.Images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = renderTarget.Data.SurfaceFormat;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};
        renderTarget.Data.Layouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;

        // Create Image View
        if (vkCreateImageView(nijiEngine.m_context.m_device, &viewInfo, nullptr, &renderTarget.Data.ImageViews[i]) != VK_SUCCESS)
        {
            assert(!"[Swapchain] Failed to Create Image View for the Swapchain Render Target");
        }
        const std::string imageViewName = "Swapchain Image View #" + std::to_string(i);
        SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_IMAGE_VIEW, renderTarget.Data.ImageViews[i], imageViewName.c_str());

        // Register as sampled image (optional, if you want to sample the swapchain)
        {
            const uint32_t slot = m_textures.capacity() + (uint32_t)i;

            VkDescriptorImageInfo imageInfo {};
            imageInfo.imageView = renderTarget.Data.ImageViews[i];
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkWriteDescriptorSet write {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = m_bindlessSet;
            write.dstBinding = BINDLESS_SAMPLED_IMAGES_BINDING;
            write.dstArrayElement = slot;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(context.m_device, 1, &write, 0, nullptr);
        }

        // Register as Storage Image
        {
            const uint32_t slot = m_textures.capacity() * MAX_MIPS + (uint32_t)i;

            VkDescriptorImageInfo imageInfo {};
            imageInfo.imageView = renderTarget.Data.ImageViews[i];
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkWriteDescriptorSet write {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = m_bindlessSet;
            write.dstBinding = BINDLESS_STORAGE_IMAGES_BINDING;
            write.dstArrayElement = slot;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(context.m_device, 1, &write, 0, nullptr);
        }

        // Create Image Presentation Semaphore
        if (vkCreateSemaphore(nijiEngine.m_context.m_device, &semaphoreInfo, nullptr, &renderTarget.Data.Semaphores[i]) != VK_SUCCESS)
        {
            assert(!"[Swapchain] Failed to Create Semaphore for Swapchain Render Target");
        }
        const std::string semaphoreName = "Swapchain Image Semaphore #" + std::to_string(i);
        SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_SEMAPHORE, renderTarget.Data.Semaphores[i], semaphoreName.c_str());
    }

    return renderTarget.Handle;
}

TextureHandle ResourceBank::create_texture(TextureDesc desc)
{
    if (desc.Format == TextureFormat::Invalid)
        assert(!"[ResourceBank] Tried Creating Texture With Invalid Texture Format");

    const VkFormat format = translate::texture_format(desc.Format);

    // Resolve mip count if auto-mip is requested
    if (desc.GenerateMips)
    {
        const uint32_t largest = std::max({desc.Size.X, desc.Size.Y, desc.Size.Z, 1u});
        const uint32_t computed = static_cast<uint32_t>(std::floor(std::log2(largest))) + 1u;
        desc.Mips = std::min(computed, MAX_MIPS);
    }

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
    if (vmaCreateImage(m_allocator, &textureInfo, &allocInfo, &texture.Data.Image, &texture.Data.Allocation, nullptr) != VK_SUCCESS)
        assert(!"[ResourceBank] Failed to Create Image Using VMA");
    // After vmaCreateBuffer in create_buffer:
    vmaSetAllocationName(m_allocator, texture.Data.Allocation, desc.Name.c_str());

    const Context& context = nijiEngine.m_context;

    SetObjectName(context.m_device, VkObjectType::VK_OBJECT_TYPE_IMAGE, texture.Data.Image, desc.Name.c_str());

    // Set Initial Layout to Undefined
    texture.Data.Layout = VK_IMAGE_LAYOUT_UNDEFINED;

    // Create Full Image View
    ImageViewDesc viewDesc {};
    viewDesc.Format = desc.Format;
    create_image_view(texture.Data.FullView, texture.Data.Image, viewDesc);

    SetObjectName(context.m_device, VK_OBJECT_TYPE_IMAGE_VIEW, texture.Data.FullView.View, (desc.Name + " [Full View]").c_str());

    // Write Sampled Images to Bindless Descriptor Set
    if (has_flag(desc.Usage, TextureUsage::Sampled))
    {
        VkDescriptorImageInfo imageInfo {};
        imageInfo.imageView = texture.Data.FullView.View;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet write {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = m_bindlessSet;
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
            ImageView& imageView = texture.Data.MippedViews[mip];

            viewDesc = {};
            viewDesc.Format = desc.Format;
            viewDesc.BaseMip = mip;
            viewDesc.Mips = 1;

            create_image_view(imageView, texture.Data.Image, viewDesc);

            SetObjectName(context.m_device, VK_OBJECT_TYPE_IMAGE_VIEW, imageView.View, (desc.Name + " [Storage Mip " + std::to_string(mip) + "]").c_str());

            VkDescriptorImageInfo imageInfo {};
            imageInfo.imageView = imageView.View;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkWriteDescriptorSet write {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_bindlessSet;
            write.dstBinding = BINDLESS_STORAGE_IMAGES_BINDING;
            write.dstArrayElement = (texture.Handle.Index - 1u) * MAX_MIPS + mip;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(context.m_device, 1, &write, 0, nullptr);
        }
    }

    // Create an Image Layout Transition Barrier
    VkImageMemoryBarrier2 imageBarrier {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    // Layout is set to Undefined at First
    // Transition to General is needed even with Unified Layouts Extension
    imageBarrier.oldLayout = texture.Data.Layout;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageBarrier.image = texture.Data.Image;
    imageBarrier.subresourceRange = texture.Data.FullView.SubRange;
    texture.Data.Layout = imageBarrier.newLayout; // Update Internal Layout to the New Layout

    //// Image Dependency Info
    // VkDependencyInfo depInfo {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    // depInfo.imageMemoryBarrierCount = 1u;
    // depInfo.pImageMemoryBarriers = &imageBarrier;

    // if (begin_upload_cmd() == false)
    //     assert(!"[Resource Bank] Failed to Begin Upload Command Buffer (create_texture)"); // Begin Recording Commands

    // VKCmdPipelineBarrier2KHR(m_uploadCmd, &depInfo);

    // if (end_upload_cmd() == false)
    //     assert(!"[Resource Bank] Failed to End Upload Command Buffer (create_texture)"); // End Recording Commands

    // TODO: RE ENABLE
    // if (desc.ShowInImGui)
    //{
    //    Sampler& sampler = m_samplers.get(nijiEngine.m_renderer.m_globalSampler);
    //
    //    texture.Data.ImGuiHandle =
    //        ImGui_ImplVulkan_AddTexture(sampler.Object, texture.Data.FullView.View, VK_IMAGE_LAYOUT_GENERAL);
    //}

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
    write.dstSet = m_bindlessSet;
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
    bufferInfo.usage = translate::buffer_usage(desc.Usage) | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Memory Allocation Info
    VmaAllocationCreateInfo allocInfo {};
    allocInfo.flags = 0x00u;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    /* Create the buffer & allocate it using VMA */
    if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &buffer.Data.Object, &buffer.Data.Allocation, nullptr) != VK_SUCCESS)
        assert(!"[ResourceBank] Failed to Create Buffer Using VMA");
    vmaSetAllocationName(m_allocator, buffer.Data.Allocation, desc.Name.c_str());

    const Context& context = nijiEngine.m_context;

    SetObjectName(context.m_device, VK_OBJECT_TYPE_BUFFER, buffer.Data.Object, desc.Name.c_str());

    // Store Address
    VkBufferDeviceAddressInfo addressInfo {};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = buffer.Data.Object;

    buffer.Data.Address = vkGetBufferDeviceAddress(context.m_device, &addressInfo);

    return buffer.Handle;
}

void ResourceBank::upload_texture(TextureHandle handle, const void* data, uint64_t size)
{
    if (size < 1u)
        assert(!"[Resource Bank] Cannot Upload to Texture if Size is 0");

    // Get TextureResource and Check if we can Upload to it
    Texture& texture = m_textures.get(handle);
    if (has_flag(texture.Desc.Usage, TextureUsage::TransferDst) == false)
        assert(!"[Resource Bank] Cannot Upload to Texture if Usage Flag 'TransferDst' is not set");

    // Resolve concrete subresource values (Desc fields may be 0)
    const VkImageAspectFlags aspect = translate::aspect_from_format(texture.Desc.Format);
    const uint32_t mipCount = std::max(1u, texture.Desc.Mips);
    const uint32_t layerCount = std::max(1u, texture.Desc.Layers);

    // Staging Buffer Creation Info
    VkBufferCreateInfo stagingBufferInfo {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    stagingBufferInfo.size = size;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Staging Memory Allocation Info
    VmaAllocationCreateInfo allocInfo {};
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // Create the Staging Buffer and Allocate it using VMA
    VkBuffer stagingBuffer {};
    VmaAllocation alloc {};
    if (vmaCreateBuffer(m_allocator, &stagingBufferInfo, &allocInfo, &stagingBuffer, &alloc, nullptr) != VK_SUCCESS)
        assert(!"[Resource Bank] Failed to Create Staging Buffer");

    // Copy Data into the Staging Buffer
    vmaCopyMemoryToAllocation(m_allocator, data, alloc, 0u, size);

    // Create Buffer to Image Copy Info
    VkBufferImageCopy copy {};
    copy.imageSubresource.aspectMask = aspect;
    copy.imageSubresource.mipLevel = 0u;
    copy.imageSubresource.baseArrayLayer = 0u;
    copy.imageSubresource.layerCount = layerCount;
    copy.imageExtent = VkExtent3D {std::max(texture.Desc.Size.X, 1u), std::max(texture.Desc.Size.Y, 1u), std::max(texture.Desc.Size.Z, 1u)};

    // Create an Image Layout Transition Barrier
    VkImageMemoryBarrier2 imageBarrier {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    // Layout is set to Undefined at First
    // Transition to General is needed even with Unified Layouts Extension
    imageBarrier.oldLayout = texture.Layout;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageBarrier.image = texture.Image;
    imageBarrier.subresourceRange = VkImageSubresourceRange {aspect, 0u, mipCount, 0u, layerCount};
    texture.Layout = imageBarrier.newLayout; // Update Internal Layout to the New Layout

    // Image Dependency Info
    VkDependencyInfo depInfo {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    depInfo.imageMemoryBarrierCount = 1u;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    if (begin_upload_cmd() == false)
        assert(!"[Resource Bank] Failed to Begin Upload Command Buffer"); // Begin Recording Commands

    VKCmdPipelineBarrier2KHR(m_uploadCmd, &depInfo);
    vkCmdCopyBufferToImage(m_uploadCmd, stagingBuffer, texture.Image, imageBarrier.newLayout, 1u, &copy);

    if (end_upload_cmd() == false)
        assert(!"[Resource Bank] Failed to End Upload Command Buffer"); // End Recording Commands

    // Destroy Staging Buffer
    vmaDestroyBuffer(m_allocator, stagingBuffer, alloc);

    //// Generate Mips if Needed
    // if (texture.Desc.GenerateMips)
    //     generate_mips(handle);
}

void ResourceBank::upload_buffer(BufferHandle handle, const void* data, uint64_t dstOffset, uint64_t size)
{
    if (size < 1u)
        assert(!"[Resource Bank] Cannot Upload to Buffer if Size is 0");

    // Get BufferResource and Check if we can Upload to it
    Buffer& buffer = m_buffers.get(handle);
    if (has_flag(buffer.Desc.Usage, BufferUsage::TransferDst) == false)
        assert(!"[Resource Bank] Cannot Upload to Buffer if Usage Flag 'TransferDst' is not set");

    // Create Staging Buffer
    VkBufferCreateInfo stagingBufferInfo {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    stagingBufferInfo.size = size;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Staging Memory Allocation Info
    VmaAllocationCreateInfo allocInfo {};
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // Create the Staging Buffer and Allocate it Using VMA
    VkBuffer stagingBuffer {};
    VmaAllocation alloc {};
    if (vmaCreateBuffer(m_allocator, &stagingBufferInfo, &allocInfo, &stagingBuffer, &alloc, nullptr) != VK_SUCCESS)
        assert(!"[Resource Bank] Failed to Create Staging Buffer");

    // Copy Data Into the Staging Buffer
    vmaCopyMemoryToAllocation(m_allocator, data, alloc, 0u, size);

    VkBufferCopy copy {};
    copy.srcOffset = 0u;
    copy.dstOffset = dstOffset;
    copy.size = size;

    if (begin_upload_cmd() == false)
        assert(!"[Resource Bank] Failed to Begin Upload Command Buffer"); // Begin recording commands

    vkCmdCopyBuffer(m_uploadCmd, stagingBuffer, buffer.Object, 1u, &copy);

    if (end_upload_cmd() == false)
        assert(!"[Resource Bank] Failed to End Upload Command Buffer"); // End Recording Commands

    // Destroy Staging Buffer
    vmaDestroyBuffer(m_allocator, stagingBuffer, alloc);
}

uint64_t ResourceBank::get_buffer_address(BufferHandle buffer) const
{
    return (uint64_t)m_buffers.get(buffer).Address;
}

uint32_t ResourceBank::get_storage_tex_index(TextureHandle texture, uint32_t mip) const
{
    return (texture.Index - 1u) * MAX_MIPS + mip;
}

void ResourceBank::destroy(ResourceHandle& handle)
{
    switch (handle.Type)
    {
    case ResourceType::Invalid:
        assert(!"[Resource Bank] Failed to Destroy Resource. Resource Type is Invalid");
        break;
    case ResourceType::RenderTarget:
        destroy_render_target((RenderTargetHandle&)handle);
        break;
    case ResourceType::Texture:
        destroy_texture((TextureHandle&)handle);
        break;
    case ResourceType::Sampler:
        destroy_sampler((SamplerHandle&)handle);
        break;
    case ResourceType::Buffer:
        destroy_buffer((BufferHandle&)handle);
        break;
    }
}

void ResourceBank::create_image_view(ImageView& imageView, VkImage image, ImageViewDesc desc)
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
    imageView.SubRange = viewInfo.subresourceRange;

    if (vkCreateImageView(context.m_device, &viewInfo, nullptr, &imageView.View) != VK_SUCCESS)
        assert(!"[ResourceBank] Failed to Create Image View");
}

void ResourceBank::generate_mips(TextureHandle handle)
{
    Texture& texture = m_textures.get(handle);

    if (texture.Desc.Mips <= 1)
        return;

    if (!has_flag(texture.Desc.Usage, TextureUsage::TransferSrc))
        assert(!"[ResourceBank] Cannot Generate Mips Without TransferSrc Usage");

    if (!has_flag(texture.Desc.Usage, TextureUsage::TransferDst))
        assert(!"[ResourceBank] Cannot Generate Mips Without TransferDst Usage");

    if (begin_upload_cmd() == false)
        assert(!"[ResourceBank] Failed to Begin Upload Command Buffer");

    int32_t mipWidth = static_cast<int32_t>(texture.Desc.Size.X);
    int32_t mipHeight = static_cast<int32_t>(texture.Desc.Size.Y);

    const uint32_t layerCount = std::max(1u, texture.Desc.Layers);

    for (uint32_t mip = 1; mip < texture.Desc.Mips; ++mip)
    {
        int32_t nextWidth = std::max(mipWidth / 2, 1);
        int32_t nextHeight = std::max(mipHeight / 2, 1);

        // Blit from previous mip to current mip
        VkImageBlit2 blit {VK_STRUCTURE_TYPE_IMAGE_BLIT_2};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = mip - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = layerCount;
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};

        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = mip;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = layerCount;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {nextWidth, nextHeight, 1};

        VkBlitImageInfo2 blitInfo {VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2};
        blitInfo.srcImage = texture.Image;
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
        blitInfo.dstImage = texture.Image;
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
        blitInfo.regionCount = 1;
        blitInfo.pRegions = &blit;
        blitInfo.filter = VK_FILTER_LINEAR;

        // Barrier: Previous Mip's Write Must be Visible Before we Read From It
        VkImageMemoryBarrier2 barrier {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.image = texture.Image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = mip - 1;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;

        VkDependencyInfo depInfo {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(m_uploadCmd, &depInfo);
        vkCmdBlitImage2(m_uploadCmd, &blitInfo);

        mipWidth = nextWidth;
        mipHeight = nextHeight;
    }

    if (end_upload_cmd() == false)
        assert(!"[ResourceBank] Failed to End Upload Command Buffer");
}

bool ResourceBank::begin_upload_cmd() const
{
    VkCommandBufferBeginInfo begin_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    return vkBeginCommandBuffer(m_uploadCmd, &begin_info) == VK_SUCCESS;
}

bool ResourceBank::end_upload_cmd() const
{
    // End the Upload Command Buffer
    vkEndCommandBuffer(m_uploadCmd);

    // Submit the Upload Commands to the Queue
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo submit {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1u;
    submit.pCommandBuffers = &m_uploadCmd;

    const Context& context = nijiEngine.m_context;

    if (vkQueueSubmit(context.m_transferQueue, 1u, &submit, m_uploadFence) != VK_SUCCESS)
        return false;

    // Wait for the Upload Commands to Complete
    if (vkWaitForFences(context.m_device, 1u, &m_uploadFence, true, UINT64_MAX) != VK_SUCCESS)
        return false;
    if (vkResetFences(context.m_device, 1u, &m_uploadFence) != VK_SUCCESS)
        return false;

    return true;
}

void ResourceBank::destroy_render_target(RenderTargetHandle& handle)
{
    // Reset RenderTargetHandle
    RenderTarget& texture = m_renderTargets.push(handle);

    const Context& context = nijiEngine.m_context;

    texture.Images.clear();
    texture.Layouts.clear();
    for (uint32_t i = 0u; i < texture.ImageCount; i++)
    {
        vkDestroyImageView(nijiEngine.m_context.m_device, texture.ImageViews[i], nullptr);
        vkDestroySemaphore(nijiEngine.m_context.m_device, texture.Semaphores[i], nullptr);
    }
    texture.ImageViews.clear();
    texture.Semaphores.clear();

    vkDestroySwapchainKHR(nijiEngine.m_context.m_device, texture.Handle, nullptr);
}

void ResourceBank::destroy_texture(TextureHandle& handle)
{
    // Reset TextureHandle and Destroy VkImage Object
    Texture& texture = m_textures.push(handle);
    vmaDestroyImage(m_allocator, texture.Image, texture.Allocation);

    const Context& context = nijiEngine.m_context;

    // Destroy Image Views
    vkDestroyImageView(context.m_device, texture.FullView.View, nullptr);
    for (ImageView& view : texture.MippedViews)
        vkDestroyImageView(context.m_device, view.View, nullptr);

    // Reset Texture Resource to Clean State
    texture = {};
}

void ResourceBank::destroy_sampler(SamplerHandle& handle)
{
    const Context& context = nijiEngine.m_context;

    // Reset SamplerHandle and Destroy VkSampler Object
    Sampler& sampler = m_samplers.push(handle);
    vkDestroySampler(context.m_device, sampler.Object, nullptr);

    // Reset Sampler Resource to Clean State
    sampler = {};
}

void ResourceBank::destroy_buffer(BufferHandle& handle)
{
    // Reset BufferHandle and Destroy VkBuffer Object
    Buffer& buffer = m_buffers.push(handle);
    vmaDestroyBuffer(m_allocator, buffer.Object, buffer.Allocation);

    // Reset Buffer Resource to Clean State
    buffer = {};
}

} // namespace niji