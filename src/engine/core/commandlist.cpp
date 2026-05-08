#include "commandlist.hpp"

#include <stdexcept>

#include "vulkan-functions.hpp"
#include "engine.hpp"
#include "context.hpp"

#include "rendering/renderer.hpp"
#include "rendering/resources/resource_handle.hpp"
#include "rendering/resource_bank.hpp"

using namespace niji;

CommandList::CommandList()
{
    // Allocate Command Buffer
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = nijiEngine.m_context.m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(nijiEngine.m_context.m_device, &allocInfo, &m_commandBuffer) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Allocate Command Buffers!");

    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_COMMAND_BUFFER, m_commandBuffer,
                  m_name);
}

void CommandList::begin_list(const char* debugName) const
{
    vkResetCommandBuffer(m_commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(m_commandBuffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer!");
}

void CommandList::begin_rendering(const RenderInfo& info, const std::string& passName,
                                  bool renderToViewport) const
{
    VkDebugUtilsLabelEXT labelInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    labelInfo.pLabelName = passName.c_str();
    labelInfo.color[0] = 0.2f;
    labelInfo.color[1] = 0.6f;
    labelInfo.color[2] = 0.9f;
    labelInfo.color[3] = 1.0f;

    VKCmdBeginDebugUtilsLabelEXT(m_commandBuffer, &labelInfo);

    Texture* viewport = nullptr;
    Texture& depthTexture = nijiEngine.m_renderer.m_resourceBank.m_textures.get(info.DepthTexture);
    RenderTarget* renderTarget = nullptr;

    VkImageView imageView {};
    VkImageLayout imageLayout {};

    if (renderToViewport)
    {
        viewport = &nijiEngine.m_renderer.m_resourceBank.m_textures.get(info.ViewportTexture);
        imageView = viewport->FullView.View;
        imageLayout = viewport->Layout;
    }
    else
    {
        renderTarget = &nijiEngine.m_renderer.m_resourceBank.m_renderTargets.get(info.RenderTarget);
        imageView = renderTarget->ImageViews[nijiEngine.m_renderer.m_imageIndex];
        imageLayout = renderTarget->Layouts[nijiEngine.m_renderer.m_imageIndex];
    }

    VkRenderingAttachmentInfoKHR colorAttachment = {};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.imageView = imageView;
    colorAttachment.imageLayout = imageLayout;
    colorAttachment.loadOp = info.TargetLoadOp;
    colorAttachment.storeOp = info.TargetStoreOp;
    colorAttachment.clearValue = info.TargetClearValue;
    VkRenderingAttachmentInfoKHR depthAttachment = {};
    if (info.HasDepth)
    {
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthAttachment.imageView = depthTexture.FullView.View;
        depthAttachment.imageLayout = depthTexture.Layout;
        depthAttachment.loadOp = info.DepthLoadOp;
        depthAttachment.storeOp = info.DepthStoreOp;
        depthAttachment.clearValue = info.DepthClearValue;
    }

    VkRenderingInfoKHR renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.renderArea = info.RenderArea;
    renderingInfo.layerCount = info.LayerCount;
    renderingInfo.colorAttachmentCount = info.TargetLoadOp != VK_ATTACHMENT_LOAD_OP_DONT_CARE ? 1 : 0;
    renderingInfo.pColorAttachments =
        info.TargetLoadOp != VK_ATTACHMENT_LOAD_OP_DONT_CARE ? &colorAttachment : nullptr;
    renderingInfo.pDepthAttachment = info.HasDepth ? &depthAttachment : nullptr;

    VKCmdBeginRenderingKHR(m_commandBuffer, &renderingInfo);
}

void CommandList::bind_pipeline(const VkPipeline& pipeline, const bool isCompute) const
{
    vkCmdBindPipeline(m_commandBuffer,
                      !isCompute ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
                      pipeline);
}

void CommandList::bind_viewport(const VkExtent2D& extent) const
{
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
}

void CommandList::bind_scissor(const VkExtent2D& extent) const
{
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
}

void CommandList::bind_vertex_buffer(uint32_t firstBinding, uint32_t bindingCount,
                                     const VkBuffer* buffers, const VkDeviceSize* offsets) const
{
    vkCmdBindVertexBuffers(m_commandBuffer, firstBinding, bindingCount, buffers, offsets);
}

void CommandList::bind_index_buffer(VkBuffer buffer, VkDeviceSize offset,
                                    VkIndexType indexType) const
{
    vkCmdBindIndexBuffer(m_commandBuffer, buffer, offset, indexType);
}

void CommandList::bind_descriptor_sets(VkPipelineBindPoint pipelineBindPoint,
                                       VkPipelineLayout layout, uint32_t firstSet,
                                       uint32_t descriptorSetCount,
                                       const VkDescriptorSet* pDescriptorSets,
                                       uint32_t dynamicOffsetCount,
                                       const uint32_t* pDynamicOffsets) const
{
    vkCmdBindDescriptorSets(m_commandBuffer, pipelineBindPoint, layout, firstSet,
                            descriptorSetCount, pDescriptorSets, dynamicOffsetCount,
                            pDynamicOffsets);
}

void CommandList::push_descriptor_set(VkPipelineBindPoint pipelineBindPoint,
                                      VkPipelineLayout layout, uint32_t set,
                                      uint32_t descriptorWriteCount,
                                      const VkWriteDescriptorSet* pDescriptorWrites) const
{
    VKCmdPushDescriptorSetKHR(m_commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount,
                              pDescriptorWrites);
}

void CommandList::draw_indexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                               int32_t vertexOffset, uint32_t firstInstance) const
{
    vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset,
                     firstInstance);
}

void CommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                       uint32_t firstInstance) const
{
    vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandList::dispatch(const uint32_t groupCountX, const uint32_t groupCountY,
                           const uint32_t groupCountZ) const
{
    vkCmdDispatch(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void CommandList::end_rendering(const RenderInfo& info) const
{
    VKCmdEndRenderingKHR(m_commandBuffer);

    //if (info.PrepareForPresent)
    //{
    //    auto& rt = info.ColorAttachment;
    //    transition_image(rt->Image, rt->Format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, TransitionType::Present);
    //    rt->CurrentLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    //}

    VKCmdEndDebugUtilsLabelEXT(m_commandBuffer);
}

void CommandList::end_list() const
{
    if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void CommandList::reset() const
{
    vkResetCommandBuffer(m_commandBuffer, 0);
}

void CommandList::cleanup()
{
    if (m_commandBuffer != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(nijiEngine.m_context.m_device, nijiEngine.m_context.m_commandPool, 1,
                             &m_commandBuffer);
        m_commandBuffer = VK_NULL_HANDLE;
    }
}

void CommandList::transition_image_layout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                                VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
                                                VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess,
                                                VkImageSubresourceRange subresource) const
{
    VkImageMemoryBarrier2 barrier {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.srcStageMask = srcStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = subresource;

    VkDependencyInfo depInfo {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    depInfo.imageMemoryBarrierCount = 1u;
    depInfo.pImageMemoryBarriers = &barrier;

    VKCmdPipelineBarrier2KHR(m_commandBuffer, &depInfo);
}