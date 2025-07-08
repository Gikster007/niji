#include "commandlist.hpp"

#include <stdexcept>

#include "vulkan-functions.hpp"
#include "../engine.hpp"

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

    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_COMMAND_BUFFER, m_commandBuffer,
                  debugName);

    /*VkDebugUtilsLabelEXT labelInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    labelInfo.pLabelName = debugName;
    labelInfo.color[0] = 0.2f;
    labelInfo.color[1] = 0.6f;
    labelInfo.color[2] = 0.9f;
    labelInfo.color[3] = 1.0f;

    auto vkCmdBeginDebugUtilsLabelEXT =
        (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(nijiEngine.m_context.m_device,
                                                                "vkCmdBeginDebugUtilsLabelEXT");
    if (vkCmdBeginDebugUtilsLabelEXT)
        vkCmdBeginDebugUtilsLabelEXT(m_commandBuffer, &labelInfo);*/
}

void CommandList::begin_rendering(const RenderInfo& info) const
{
    std::vector<VkRenderingAttachmentInfoKHR> colorAttachments(info.ColorAttachments.size());
    for (size_t i = 0; i < info.ColorAttachments.size(); ++i)
    {
        const auto& src = info.ColorAttachments[i];
        auto& dst = colorAttachments[i];

        dst.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        dst.imageView = src.ImageView;
        dst.imageLayout = src.ImageLayout;
        dst.loadOp = src.LoadOp;
        dst.storeOp = src.StoreOp;
        dst.clearValue = src.ClearValue;
    }

    VkRenderingAttachmentInfoKHR depthAttachment{};
    if (info.HasDepth)
    {
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthAttachment.imageView = info.DepthAttachment.ImageView;
        depthAttachment.imageLayout = info.DepthAttachment.ImageLayout;
        depthAttachment.loadOp = info.DepthAttachment.LoadOp;
        depthAttachment.storeOp = info.DepthAttachment.StoreOp;
        depthAttachment.clearValue = info.DepthAttachment.ClearValue;
    }

    VkRenderingInfoKHR renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.renderArea = info.RenderArea;
    renderingInfo.layerCount = info.LayerCount;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderingInfo.pColorAttachments = colorAttachments.data();
    renderingInfo.pDepthAttachment = info.HasDepth ? &depthAttachment : nullptr;

    for (int i = 0; i < colorAttachments.size(); i++)
    {
        auto& rt = colorAttachments[i];
        auto& infoRT = info.ColorAttachments[i];
        transition_image(infoRT.Image, infoRT.Format, VK_IMAGE_LAYOUT_UNDEFINED, rt.imageLayout,
                         TransitionType::ColorAttachment);
    }

    auto& depthInfoRT = info.DepthAttachment;
    transition_image(depthInfoRT.Image, depthInfoRT.Format, VK_IMAGE_LAYOUT_UNDEFINED,
                     depthAttachment.imageLayout, TransitionType::DepthStencilAttachment);

    VKCmdBeginRenderingKHR(m_commandBuffer, &renderingInfo);
}

void CommandList::bind_pipeline(const VkPipeline& pipeline) const
{
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void CommandList::bind_viewport(const VkViewport& viewport) const
{
    vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
}

void CommandList::bind_scissor(const VkRect2D& scissor) const
{
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
                                      const VkWriteDescriptorSet* pDescriptorWrites)
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

void CommandList::end_rendering(const RenderInfo& info) const
{
    VKCmdEndRenderingKHR(m_commandBuffer);

    for (int i = 0; i < info.ColorAttachments.size(); i++)
    {
        auto& rt = info.ColorAttachments[i];
        transition_image(rt.Image, rt.Format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, TransitionType::Present);
    }
}

void CommandList::end_list() const
{
    /*auto vkCmdEndDebugUtilsLabelEXT =
        (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(nijiEngine.m_context.m_device,
                                                            "vkCmdEndDebugUtilsLabelEXT");
    if (vkCmdEndDebugUtilsLabelEXT)
        vkCmdEndDebugUtilsLabelEXT(m_commandBuffer);*/

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

void CommandList::transition_image(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                   VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                   VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                   VkImageAspectFlags aspectMask, uint32_t mipLevels,
                                   uint32_t layerCount) const
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = srcStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;
    dependencyInfo.dependencyFlags = 0;

    VKCmdPipelineBarrier2KHR(m_commandBuffer, &dependencyInfo);
}

void CommandList::transition_image(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                   VkImageLayout newLayout, TransitionType usage,
                                   uint32_t mipLevels, uint32_t layerCount) const
{
    VkAccessFlags srcAccessMask = 0;
    VkAccessFlags dstAccessMask = 0;
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkImageAspectFlags aspectMask = 0;

    switch (usage)
    {
    case TransitionType::ColorAttachment:
        dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        break;

    case TransitionType::DepthStencilAttachment:
        dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (nijiEngine.m_context.has_stencil_component(format))
        {
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        break;

    case TransitionType::Present:
        srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        break;

    case TransitionType::ShaderRead:
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        break;

    case TransitionType::TransferDst:
        dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        break;

    case TransitionType::TransferSrc:
        dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    }

    transition_image(image, oldLayout, newLayout, srcAccessMask, dstAccessMask, srcStage, dstStage,
                     aspectMask, mipLevels, layerCount);
}
