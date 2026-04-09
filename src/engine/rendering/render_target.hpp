#pragma once

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

namespace niji
{

struct RenderTargetDesc
{
    VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkClearValue ClearValue = {};
    VkFormat Format = {};

    uint32_t Width = 1;
    uint32_t Height = 1;

    bool ShowInImGui = false;

    char* Name = nullptr;
};

// TODO: Refactor (create RenderTargetInfo struct which holds construct info that will be passed in
// the constructor of RenderTarget)
struct RenderTarget
{
    RenderTarget() = default;
    // Swapchain Image RT
    RenderTarget(VkImage image, VkImageView view, VkFormat Format = VK_FORMAT_UNDEFINED,
                 VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED, VkAttachmentLoadOp load = VK_ATTACHMENT_LOAD_OP_CLEAR,
                 VkAttachmentStoreOp store = VK_ATTACHMENT_STORE_OP_STORE, VkClearValue clear = {});
    // Custom RT
    RenderTarget(RenderTargetDesc desc);
    void cleanup() const;

    VkImage Image = VK_NULL_HANDLE;
    VkImageView ImageView = VK_NULL_HANDLE;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageLayout CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkClearValue ClearValue = {};

    char* Name = nullptr;

    // Used only for Custom RTs
    VmaAllocation Allocation = {};
    VkDescriptorSet ImGuiHandle = {};
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

    RenderTarget* ColorAttachment = nullptr;
    RenderTarget* DepthAttachment = nullptr;
    RenderTarget* ViewportTarget = nullptr;

    bool HasDepth = false;
    bool PrepareForPresent = false;
};

} // namespace niji