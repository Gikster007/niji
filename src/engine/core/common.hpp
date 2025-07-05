#pragma once

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

namespace niji
{
template <typename HandleType>
void SetObjectName(VkDevice device, VkObjectType type, HandleType handle, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = type;
    nameInfo.objectHandle = reinterpret_cast<uint64_t>(handle);
    nameInfo.pObjectName = name;

    auto func =
        (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device,
                                                              "vkSetDebugUtilsObjectNameEXT");
    if (func)
    {
        func(device, &nameInfo);
    }
}

struct BufferDesc
{
    VkDeviceSize Size = {};
    enum class BufferUsage
    {
        Invalid,
        Vertex,
        Index,
    } Usage = {};
    bool IsPersistent = false;
    char* Name = "Unknown Buffer";
};

struct Buffer
{
    Buffer() = default;
    Buffer(BufferDesc& desc, void* data);

    VkBuffer Handle = {};
    VmaAllocation BufferAllocation = {};
    BufferDesc Desc = {};
};

struct NijiTexture
{
    VkImage TextureImage = {};
    VkImageView TextureImageView = {};
    VmaAllocation TextureImageAllocation = {};
};

struct NijiUBO
{
    std::vector<VkBuffer> UniformBuffers = {};
    std::vector<VmaAllocation> UniformBuffersAllocations = {};
    std::vector<void*> UniformBuffersMapped = {};
};

enum class TransitionType
{
    ColorAttachment,
    DepthStencilAttachment,
    Present,
    ShaderRead,  // sampled image
    TransferDst, // for upload
    TransferSrc  // for copy
};

struct RenderTarget
{
    RenderTarget() = default;
    RenderTarget(VkImage image, VkImageView view, VkFormat format = VK_FORMAT_UNDEFINED,
                 VkImageLayout layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
                 VkAttachmentLoadOp load = VK_ATTACHMENT_LOAD_OP_CLEAR,
                 VkAttachmentStoreOp store = VK_ATTACHMENT_STORE_OP_STORE, VkClearValue clear = {})
        : Image(image), ImageView(view), Format(format), ImageLayout(layout), LoadOp(load),
          StoreOp(store), ClearValue(clear)
    {
    }

    VkImage Image = VK_NULL_HANDLE;
    VkImageView ImageView = VK_NULL_HANDLE;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkClearValue ClearValue = {};
};

struct RenderInfo
{
    RenderInfo() = default;
    RenderInfo(const VkExtent2D& extent)
    {
        RenderArea.offset = {0, 0};
        RenderArea.extent = extent;
    }

    VkRect2D RenderArea = {{0, 0}, {0, 0}};
    uint32_t LayerCount = 1;

    std::vector<RenderTarget> ColorAttachments = {};
    RenderTarget DepthAttachment = {};

    bool HasDepth = false;
};

} // namespace niji
