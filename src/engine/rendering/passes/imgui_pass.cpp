#include "imgui_pass.hpp"

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <imgui.h>

#include "../../engine.hpp"
#include "../renderer.hpp"

using namespace niji;

void ImGuiPass::init(Swapchain& swapchain, Descriptor& globalDescriptor)
{
    m_name = "ImGui Pass";

    // 1. Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // 2. Set style (optional)
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.FontScaleMain = 1.25f;

    // 3. Init ImGui for GLFW
    ImGui_ImplGlfw_InitForVulkan(nijiEngine.m_context.m_window, true);

    // Descriptor Pool Needed for ImGui
    {
        VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                             {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                             {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                             {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        vkCreateDescriptorPool(nijiEngine.m_context.m_device, &pool_info, nullptr,
                               &m_imguiDescriptorPool);
    }

    QueueFamilyIndices indices =
        niji::QueueFamilyIndices::find_queue_families(nijiEngine.m_context.m_physicalDevice,
                                                      nijiEngine.m_context.m_surface);
    // 4. Init ImGui for Vulkan
    VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipelineRenderingInfo.pNext = nullptr;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    VkFormat colorFormat = swapchain.m_format;
    pipelineRenderingInfo.pColorAttachmentFormats = &colorFormat;
    pipelineRenderingInfo.depthAttachmentFormat = nijiEngine.m_context.find_depth_format();
    pipelineRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = nijiEngine.m_context.m_instance;
    init_info.PhysicalDevice = nijiEngine.m_context.m_physicalDevice;
    init_info.Device = nijiEngine.m_context.m_device;
    init_info.QueueFamily = indices.GraphicsFamily.value();
    init_info.Queue = nijiEngine.m_context.m_graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_imguiDescriptorPool;
    init_info.RenderPass = VK_NULL_HANDLE;
    init_info.MinImageCount = 2;
    init_info.ImageCount = swapchain.m_images.size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.UseDynamicRendering = true;
    init_info.PipelineRenderingCreateInfo = pipelineRenderingInfo;

    ImGui_ImplVulkan_Init(&init_info);
}

void ImGuiPass::update_impl(Renderer& renderer, CommandList& cmd)
{
    // Draw Editor
    nijiEngine.m_editor.render();
}

void ImGuiPass::record(Renderer& renderer, CommandList& cmd, RenderInfo& info)
{
    info.ColorAttachment->StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    info.ColorAttachment->LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    info.DepthAttachment->StoreOp = VK_ATTACHMENT_STORE_OP_NONE;
    info.DepthAttachment->LoadOp = VK_ATTACHMENT_LOAD_OP_NONE_KHR;

    {
        TransitionInfo before;
        if (info.ColorAttachment->CurrentLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            before = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED};
        else if (info.ColorAttachment->CurrentLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            before = {VK_PIPELINE_STAGE_NONE, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

        TransitionInfo after = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

        cmd.transition_image_explicit(*info.ColorAttachment, before, after, aspect, 1, 1);
    }

    {
        TransitionInfo before;
        if (info.ViewportTarget->CurrentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            before = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        TransitionInfo after = {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

        cmd.transition_image_explicit(*info.ViewportTarget, before, after, aspect, 1, 1);
    }

    auto& viewportRT = renderer.m_viewportTargets[renderer.m_imageIndex];

    ImGui::Begin("Debug Image");
    auto& size = ImGui::GetContentRegionAvail();
    ImGui::Image(viewportRT.ImGuiHandle, size);
    ImGui::End();

    cmd.begin_rendering(info, m_name, false);

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd.m_commandBuffer);

    cmd.end_rendering(info);
}

void ImGuiPass::cleanup()
{
    base_cleanup();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(nijiEngine.m_context.m_device, m_imguiDescriptorPool, nullptr);
}