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

    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_COMMAND_BUFFER, m_commandBuffer, m_name);
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

    /*SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_COMMAND_BUFFER, m_commandBuffer,
                  debugName);

    VkDebugUtilsLabelEXT labelInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    labelInfo.pLabelName = debugName;
    labelInfo.color[0] = 0.2f;
    labelInfo.color[1] = 0.6f;
    labelInfo.color[2] = 0.9f;
    labelInfo.color[3] = 1.0f;

    if (VKCmdBeginDebugUtilsLabelEXT)
        VKCmdBeginDebugUtilsLabelEXT(m_commandBuffer, &labelInfo);*/
}

void CommandList::begin_rendering(const RenderInfo& info, const std::string& passName) const
{
    VkDebugUtilsLabelEXT labelInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    labelInfo.pLabelName = passName.c_str();
    labelInfo.color[0] = 0.2f;
    labelInfo.color[1] = 0.6f;
    labelInfo.color[2] = 0.9f;
    labelInfo.color[3] = 1.0f;

    VKCmdBeginDebugUtilsLabelEXT(m_commandBuffer, &labelInfo);

    VkRenderingAttachmentInfoKHR colorAttachment = {};
    //const auto& src = info.ColorAttachment;

    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.imageView = info.ColorAttachment->ImageView;
    colorAttachment.imageLayout = info.ColorAttachment->CurrentLayout;
    colorAttachment.loadOp = info.ColorAttachment->LoadOp;
    colorAttachment.storeOp = info.ColorAttachment->StoreOp;
    colorAttachment.clearValue = info.ColorAttachment->ClearValue;
    VkRenderingAttachmentInfoKHR depthAttachment{};
    if (info.HasDepth)
    {
        /*VkImageLayout newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        if (info.DepthAttachment.StoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE)
            newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;*/

        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthAttachment.imageView = info.DepthAttachment->ImageView;
        depthAttachment.imageLayout = info.DepthAttachment->CurrentLayout;
        depthAttachment.loadOp = info.DepthAttachment->LoadOp;
        depthAttachment.storeOp = info.DepthAttachment->StoreOp;
        depthAttachment.clearValue = info.DepthAttachment->ClearValue;
    }

    VkRenderingInfoKHR renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.renderArea = info.RenderArea;
    renderingInfo.layerCount = info.LayerCount;
    renderingInfo.colorAttachmentCount =
        info.ColorAttachment->LoadOp != VK_ATTACHMENT_LOAD_OP_DONT_CARE ? 1 : 0;
    renderingInfo.pColorAttachments =
        info.ColorAttachment->LoadOp != VK_ATTACHMENT_LOAD_OP_DONT_CARE ? &colorAttachment
                                                                        : nullptr;
    renderingInfo.pDepthAttachment = info.HasDepth ? &depthAttachment : nullptr;

   /* auto& infoRT = info.ColorAttachment;
    transition_image(infoRT.Image, infoRT.Format, infoRT.ImageLayout, colorAttachment.imageLayout,
                     TransitionType::ColorAttachment);*/

    /*auto& depthInfoRT = info.DepthAttachment;
    transition_image(depthInfoRT.Image, depthInfoRT.Format, depthInfoRT.ImageLayout,
                     depthAttachment.imageLayout, TransitionType::DepthStencilAttachment);*/

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

    if (info.PrepareForPresent)
    {
        auto& rt = info.ColorAttachment;
        transition_image(rt->Image, rt->Format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, TransitionType::Present);
        rt->CurrentLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

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

void CommandList::transition_image_explicit(RenderTarget& rt, TransitionInfo before,
                                            TransitionInfo after, VkImageAspectFlags aspectMask,
                                            uint32_t mipLevels, uint32_t layerCount) const
{
    if (before.Layout == after.Layout)
        return; // No transition needed

    rt.CurrentLayout = after.Layout;

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = before.Stage;
    barrier.srcAccessMask = before.Access;
    barrier.dstStageMask = after.Stage;
    barrier.dstAccessMask = after.Access;
    barrier.oldLayout = before.Layout;
    barrier.newLayout = after.Layout;
    barrier.image = rt.Image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    VKCmdPipelineBarrier2KHR(m_commandBuffer, &dependencyInfo);
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

    case TransitionType::DepthStencilAttachmentWrite:
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
        dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
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
