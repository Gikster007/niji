#include "renderer.hpp"

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>

#include <vk_mem_alloc.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

using namespace niji;

#include "core/components/render-components.hpp"
#include "core/components/transform.hpp"
#include "core/vulkan-functions.hpp"

#include "swapchain.hpp"
#include "engine.hpp"

Renderer::Renderer()
{
    m_context = &nijiEngine.m_context;
    init();
}

Renderer::~Renderer()
{
}

void Renderer::init()
{
    // 1. Create global descriptor set layout (set = 0)
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        // binding 0 = matrices
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_context->m_device, &layoutInfo, nullptr,
                                    &m_globalSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create global descriptor set layout.");
    }

    // 2. Create Descriptor Pool
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(m_context->m_device, &poolInfo, nullptr, &m_descriptorPool) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create global descriptor pool.");
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        // 3. Allocate global descriptor set
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_globalSetLayout;

        if (vkAllocateDescriptorSets(m_context->m_device, &allocInfo, &m_globalDescriptorSet[i]) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate global descriptor set.");
        }

        // 4. Write to global descriptor set
        UniformBufferObject ubo = {};
        BufferDesc bufferDesc = {};
        bufferDesc.IsPersistent = true;
        bufferDesc.Name = "UBO";
        bufferDesc.Size = sizeof(UniformBufferObject);
        bufferDesc.Usage = BufferDesc::BufferUsage::Uniform;
        m_ubos[i] = Buffer(bufferDesc, &ubo);

        VkDescriptorBufferInfo cameraInfo = {};
        cameraInfo.buffer = m_ubos[i].Handle;
        cameraInfo.offset = 0;
        cameraInfo.range = sizeof(UniformBufferObject);

        std::vector<VkWriteDescriptorSet> writes = {};
        VkWriteDescriptorSet write1 = {};
        write1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write1.dstSet = m_globalDescriptorSet[i];
        write1.dstBinding = 0;
        write1.dstArrayElement = 0;
        write1.descriptorCount = 1;
        write1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write1.pBufferInfo = &cameraInfo;
        writes.push_back(write1);

        vkUpdateDescriptorSets(m_context->m_device, static_cast<uint32_t>(writes.size()),
                               writes.data(), 0, nullptr);
    }

    // ImGui Init
    {
        // 1. Create ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        // 2. Set style (optional)
        ImGui::StyleColorsDark();

        // 3. Init ImGui for GLFW
        ImGui_ImplGlfw_InitForVulkan(m_context->m_window, true);

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

            vkCreateDescriptorPool(m_context->m_device, &pool_info, nullptr,
                                   &m_imguiDescriptorPool);
        }

        QueueFamilyIndices indices =
            niji::QueueFamilyIndices::find_queue_families(m_context->m_physicalDevice,
                                                          m_context->m_surface);
        // 4. Init ImGui for Vulkan
        VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {};
        pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        pipelineRenderingInfo.pNext = nullptr;
        pipelineRenderingInfo.colorAttachmentCount = 1;
        VkFormat colorFormat = m_swapchain.m_format;
        pipelineRenderingInfo.pColorAttachmentFormats = &colorFormat;
        pipelineRenderingInfo.depthAttachmentFormat = nijiEngine.m_context.find_depth_format();
        pipelineRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_context->m_instance;
        init_info.PhysicalDevice = nijiEngine.m_context.m_physicalDevice;
        init_info.Device = nijiEngine.m_context.m_device;
        init_info.QueueFamily = indices.GraphicsFamily.value();
        init_info.Queue = m_context->m_graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = m_imguiDescriptorPool;
        init_info.RenderPass = VK_NULL_HANDLE;
        init_info.MinImageCount = 2;
        init_info.ImageCount = m_swapchain.m_images.size();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.UseDynamicRendering = true;
        init_info.PipelineRenderingCreateInfo = pipelineRenderingInfo;

        ImGui_ImplVulkan_Init(&init_info);
    }

    // Render Passes
    {
        m_renderPasses.push_back(std::make_unique<ForwardPass>());
    }
    {
        for (auto& pass : m_renderPasses)
        {
            pass->init(m_swapchain, m_globalSetLayout);
        }
    }

    // Create Command Buffers
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    create_sync_objects();
}

void Renderer::update(const float dt)
{
    update_uniform_buffer(m_currentFrame);

    for (auto& pass : m_renderPasses)
    {
        pass->update(*this);
    }
}

void Renderer::render()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    VkFence frameFence = m_inFlightFences[m_currentFrame];
    vkWaitForFences(m_context->m_device, 1, &frameFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_context->m_device, 1, &frameFence);

    VkSemaphore acquireSemaphore = m_imageAvailableSemaphores[m_currentFrame];

    VkResult result = vkAcquireNextImageKHR(m_context->m_device, m_swapchain.m_object, UINT64_MAX,
                                            acquireSemaphore, VK_NULL_HANDLE, &m_imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        m_swapchain.recreate();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to Acquire Swap Chain Image!");
    }

    auto& cmd = m_commandBuffers[m_currentFrame];
    cmd.begin_list("Frame Commmand Buffer");

    for (auto& pass : m_renderPasses)
    {
        pass->record(*this, cmd);
    }

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd.m_commandBuffer);

    cmd.end_list();

    VkSemaphore submitSemaphore = m_renderFinishedSemaphores[m_imageIndex];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &acquireSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame].m_commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &submitSemaphore;

    if (vkQueueSubmit(m_context->m_graphicsQueue, 1, &submitInfo,
                      m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to Submit Draw Command Buffer!");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &submitSemaphore;

    VkSwapchainKHR swapChains[] = {m_swapchain.m_object};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_context->m_presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        m_context->m_framebufferResized)
    {
        m_swapchain.recreate();
        m_context->m_framebufferResized = false;
    }
    else if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to Present Swap Chain Image!");

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::cleanup()
{
    m_swapchain.cleanup();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    auto view = nijiEngine.ecs.m_registry.view<Transform, MeshComponent>();
    for (auto&& [entity, trans, mesh] : view.each())
    {
        auto& model = mesh.Model;
        auto& modelMesh = model->m_meshes[mesh.MeshID];
        auto& modelMaterial = model->m_materials[mesh.MeshID];
        model->cleanup();
    }

    for (int i = 0; i < m_ubos.size(); i++)
    {
        m_ubos[i].cleanup();
    }

    for (auto& pass : m_renderPasses)
    {
        pass->cleanup();
        pass.release();
    }
    m_renderPasses.clear();

    {
        vkDestroyDescriptorSetLayout(m_context->m_device, m_globalSetLayout, nullptr);
        vkDestroyDescriptorPool(m_context->m_device, m_descriptorPool, nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_context->m_device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_context->m_device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_context->m_device, m_inFlightFences[i], nullptr);
    }
}

void Renderer::create_sync_objects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(m_context->m_device, &semaphoreInfo, nullptr,
                              &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_context->m_device, &semaphoreInfo, nullptr,
                              &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_context->m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) !=
                VK_SUCCESS)
        {
            throw std::runtime_error("Failed to Create Semaphores and Fences!");
        }
    }
}

void Renderer::update_uniform_buffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo = {};

    ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.Proj = glm::perspective(glm::radians(45.0f),
                                m_swapchain.m_extent.width / (float)m_swapchain.m_extent.height,
                                0.1f, 10.0f);
    ubo.Proj[1][1] *= -1;

    memcpy(m_ubos[currentImage].Data, &ubo, sizeof(ubo));
}