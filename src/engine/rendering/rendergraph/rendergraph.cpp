#include "rendergraph.hpp"

#include "nodes/compute_node.hpp"

#include "engine.hpp"
#include "core/context.hpp"

#include "rendering/renderer.hpp"
#include "rendering/resource_bank.hpp"
#include "rendering/resources/render_target.hpp"
#include "rendering/utils/translate.hpp"

namespace niji
{

template <typename T>
T div_up(const T x, const T y)
{
    return (x + y - 1) / y;
}

void RenderGraph::init()
{
    // Allocate Frame Resources Ring Buffer
    m_resources = new FrameResources[m_maxFrames] {};
    m_currentFrame = 0u;

    // Command Buffer Alloc Info
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = nijiEngine.m_context.m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1u;

    // Semaphore Info
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Fence Info
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0u; i < m_maxFrames; i++)
    {
        // Allocate Command Buffers
        if (vkAllocateCommandBuffers(nijiEngine.m_context.m_device, &allocInfo, &m_resources[i].Cmd) != VK_SUCCESS)
        {
            printf("Failed to Allocate Render Graph Command Buffers!");
            return;
        }
        std::string cmdName = "Render Graph Command Buffer #";
        cmdName += std::to_string(i);
        SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_COMMAND_BUFFER, m_resources[i].Cmd, cmdName.c_str());

        // Create Semaphores and Fences
        if (vkCreateSemaphore(nijiEngine.m_context.m_device, &semaphoreInfo, nullptr, &m_resources[i].Semaphore) != VK_SUCCESS ||
            vkCreateFence(nijiEngine.m_context.m_device, &fenceInfo, nullptr, &m_resources[i].Fence) != VK_SUCCESS)
        {
            printf("Failed to Create Semaphores and Fences!");
            return;
        }

        std::string imageAvailableName = "Render Graph Semaphore #";
        imageAvailableName += std::to_string(i);
        SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_SEMAPHORE, m_resources[i].Semaphore, imageAvailableName.c_str());

        std::string inFlightFenceName = "Render Graph Fence #";
        inFlightFenceName += std::to_string(i);
        SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_FENCE, m_resources[i].Fence, inFlightFenceName.c_str());
    }
}

void RenderGraph::new_frame()
{
    // In Nanoseconds (one second)
    constexpr uint64_t TIMEOUT = 1'000'000'000u;

    // Wait For this Frame to be Out of Flight Before re-using its Resources
    if (vkWaitForFences(nijiEngine.m_context.m_device, 1u, &current_frame().Fence, true, TIMEOUT) != VK_SUCCESS)
    {
        printf("failed while waiting for graph in-flight fence.");
        return;
    }

    for (Node* old_node : m_nodes)
    {
        delete old_node;
    }

    m_renderTarget = RenderTargetHandle();

    // Reset the Nodes
    m_nodes.clear();
    m_nodes.reserve(128u);
}

FrameResources& RenderGraph::current_frame()
{
    return m_resources[m_currentFrame];
}

void RenderGraph::next_frame()
{
    m_currentFrame++;
    if (m_currentFrame >= m_maxFrames)
        m_currentFrame = 0u;
}

ComputeNode& RenderGraph::add_compute_node(std::string_view label, std::string_view shader_path)
{
    ComputeNode* node = new ComputeNode(label, shader_path);
    m_nodes.emplace_back((Node*)node);
    return *node;
}

void RenderGraph::set_render_target(RenderTargetHandle& rt)
{
    m_renderTarget = rt;
}

void RenderGraph::execute()
{
    const FrameResources& frame = current_frame();

    RenderTarget& rt = nijiEngine.m_renderer.m_resourceBank.m_renderTargets.get(m_renderTarget);
    VkResult result = vkAcquireNextImageKHR(nijiEngine.m_context.m_device, rt.Handle, UINT64_MAX, frame.Semaphore, VK_NULL_HANDLE, &rt.CurrentImage);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // m_swapchain.recreate();
        //  TODO: re-implement swapchain recreation
        printf("[Renderer] acquire returned OUT_OF_DATE\n");
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to Acquire Swap Chain Image!");
    }

    vkResetCommandBuffer(frame.Cmd, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(frame.Cmd, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer!");

    // Transition Render Target to General Layout on New Frame Begin
    {
        VkImageMemoryBarrier2 toGeneral {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        toGeneral.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        toGeneral.srcAccessMask = VK_ACCESS_2_NONE;
        toGeneral.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        toGeneral.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
        toGeneral.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // discard old contents, we render fresh
        toGeneral.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        toGeneral.image = rt.Images[rt.CurrentImage]; // your swapchain VkImage for this index
        toGeneral.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};

        VkDependencyInfo depGeneral {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        depGeneral.imageMemoryBarrierCount = 1u;
        depGeneral.pImageMemoryBarriers = &toGeneral;

        vkCmdPipelineBarrier2(frame.Cmd, &depGeneral);
        rt.Layouts[rt.CurrentImage] = VK_IMAGE_LAYOUT_GENERAL;
    }

    // Rebuilt each frame — tracks the last access of each resource
    struct ResourceAccess
    {
        DependencyUsage Usage;
        DependencyStages Stages;
    };
    std::unordered_map<uint32_t, ResourceAccess> lastAccess;

    for (Node* node : m_nodes)
    {
        if (node == nullptr)
        {
            printf("Invalid Node!");
            return;
        }

        // Gather hazards for this pass into a single coalesced barrier

        VkPipelineStageFlags2 srcStages = VK_PIPELINE_STAGE_2_NONE;
        VkPipelineStageFlags2 dstStages = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 srcAccess = VK_ACCESS_2_NONE;
        VkAccessFlags2 dstAccess = VK_ACCESS_2_NONE;
        bool needsBarrier = false;

        for (Dependency& dep : node->m_dependencies)
        {
            auto it = lastAccess.find(dep.Resource.raw());
            if (it != lastAccess.end())
            {
                const bool prevWrite = it->second.Usage == DependencyUsage::ReadWrite;
                const bool currWrite = dep.Usage == DependencyUsage::ReadWrite;

                // Hazard unless both are reads
                if (prevWrite || currWrite)
                {
                    needsBarrier = true;
                    srcStages |= translate::to_vk_stages(it->second.Stages);
                    dstStages |= translate::to_vk_stages(dep.Stages);
                    srcAccess |= translate::to_vk_access(it->second.Usage);
                    dstAccess |= translate::to_vk_access(dep.Usage);
                }
            }

            lastAccess[dep.Resource.raw()] = {dep.Usage, dep.Stages};
        }

        // ----- Emit the barrier before the dispatch -----
        if (needsBarrier)
        {
            VkMemoryBarrier2 barrier {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
            barrier.srcStageMask = srcStages;
            barrier.srcAccessMask = srcAccess;
            barrier.dstStageMask = dstStages;
            barrier.dstAccessMask = dstAccess;

            VkDependencyInfo depInfo {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
            depInfo.memoryBarrierCount = 1u;
            depInfo.pMemoryBarriers = &barrier;

            vkCmdPipelineBarrier2(frame.Cmd, &depInfo);
        }

        // get pipeline (create or fetch cached one)
        const ComputeNode& compNode = *(const ComputeNode*)node;
        const Pipeline& pipeline = m_pipelineCache.get_pipeline("shaders/spirv/", compNode);
        vkCmdBindPipeline(frame.Cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.Object);

        // Bind Bindless Descriptor Set
        vkCmdBindDescriptorSets(frame.Cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.Layout, 0u, 1u, &nijiEngine.m_renderer.m_resourceBank.m_bindlessSet, 0u, nullptr);

        // Upload Push Constants
        if (compNode.m_rtInjectOffset != UINT32_MAX)
        {   
            const uint32_t targetSlot = nijiEngine.m_renderer.m_resourceBank.m_textures.capacity() * MAX_MIPS + rt.CurrentImage;

            std::memcpy(node->m_pcData + compNode.m_rtInjectOffset, &targetSlot, sizeof(uint32_t));
        }
        if (node->m_rangeSize != 0u)
            vkCmdPushConstants(frame.Cmd, pipeline.Layout, VK_SHADER_STAGE_COMPUTE_BIT, node->m_rangeOffset, node->m_rangeSize, node->m_pcData);

        // Calculate the Dispatch Size
        const uint32_t dispatchX = div_up(compNode.m_workX, compNode.m_groupX);
        const uint32_t dispatchY = div_up(compNode.m_workY, compNode.m_groupY);
        const uint32_t dispatchZ = div_up(compNode.m_workZ, compNode.m_groupZ);

        // Dispatch
        vkCmdDispatch(frame.Cmd, dispatchX, dispatchY, dispatchZ);
    }

    // render target pipeline barrier
    {
        VkImageMemoryBarrier2 toPresent {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        toPresent.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        toPresent.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
        toPresent.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        toPresent.dstAccessMask = VK_ACCESS_2_NONE;
        toPresent.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        toPresent.image = rt.Images[rt.CurrentImage];
        toPresent.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};

        VkDependencyInfo depPresent {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        depPresent.imageMemoryBarrierCount = 1u;
        depPresent.pImageMemoryBarriers = &toPresent;

        vkCmdPipelineBarrier2(frame.Cmd, &depPresent);
        rt.Layouts[rt.CurrentImage] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    vkEndCommandBuffer(frame.Cmd);

    // TODO: Remember to add vkWaitForFence call
    if (vkResetFences(nijiEngine.m_context.m_device, 1u, &frame.Fence) != VK_SUCCESS)
    {
        printf("Failed to Reset Graph In-Flight Fence!");
        return;
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &frame.Semaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.Cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &rt.Semaphores[rt.CurrentImage];

    if (vkQueueSubmit(nijiEngine.m_context.m_graphicsQueue, 1, &submitInfo, frame.Fence) != VK_SUCCESS)
    {
        printf("Failed to Submit Draw Command Buffer!");
        return;
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &rt.Semaphores[rt.CurrentImage];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &rt.Handle;
    presentInfo.pImageIndices = &rt.CurrentImage;

    result = vkQueuePresentKHR(nijiEngine.m_context.m_presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || nijiEngine.m_context.m_framebufferResized)
    {
        // m_swapchain.recreate();
        //  TODO: re-implement swapchain recreation
        nijiEngine.m_context.m_framebufferResized = false;
    }
    else if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to Present Swap Chain Image!");

    next_frame();
}

} // namespace niji