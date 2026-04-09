#include "render_target.hpp"

#include <vk_mem_alloc.h>
#include <imgui_impl_vulkan.h>

#include "../core/common.hpp"

#include "engine.hpp"

namespace niji
{

RenderTarget::RenderTarget(VkImage image, VkImageView view, VkFormat Format, VkImageLayout layout, VkAttachmentLoadOp load,
                           VkAttachmentStoreOp store, VkClearValue clear)
    : Image(image), ImageView(view), Format(Format), ImageLayout(layout), LoadOp(load), StoreOp(store), ClearValue(clear)
{
    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_IMAGE, Image, Name);
}

RenderTarget::RenderTarget(RenderTargetDesc desc)
{
    Format = desc.Format;
    LoadOp = desc.LoadOp;
    StoreOp = desc.StoreOp;
    ClearValue = desc.ClearValue;
    Name = desc.Name;

    nijiEngine.m_context.create_image(desc.Width, desc.Height, 1, 1, Format, VK_IMAGE_TILING_OPTIMAL,
                                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_AUTO, 0,
                                      Image, Allocation);

    ImageView = nijiEngine.m_context.create_image_view(Image, Format, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1);

    ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (desc.ShowInImGui)
    {
        ImGuiHandle = ImGui_ImplVulkan_AddTexture(nijiEngine.m_context.m_globalSampler.Handle, ImageView,
                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_IMAGE, Image, Name);
}

void RenderTarget::cleanup() const
{
    vkDestroyImageView(nijiEngine.m_context.m_device, ImageView, nullptr);
    vmaDestroyImage(nijiEngine.m_context.m_allocator, Image, Allocation);
}

} // namespace niji