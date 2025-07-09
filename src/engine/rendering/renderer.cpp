#include "renderer.hpp"

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>

#include <vk_mem_alloc.h>

#include <imgui.h>
#include <stb_image.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

using namespace niji;

#include "../../app/camera_system.hpp"

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
    {
        int width = -1, height = -1, channels = -1;
        unsigned char* imageData = nullptr;

        imageData = stbi_load("assets/missing.png", &width, &height, &channels, STBI_rgb_alpha);

        if (!imageData)
        {
            printf("[Renderer] Failed to load Fallback Texture");
        }

        m_fallbackTexture =
            nijiEngine.m_context.create_texture_image(imageData, width, height, channels);
        nijiEngine.m_context.create_texture_image_view(m_fallbackTexture);

        m_fallbackTexture.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        m_fallbackTexture.ImageInfo.imageView = m_fallbackTexture.TextureImageView;
        m_fallbackTexture.ImageInfo.sampler = VK_NULL_HANDLE;
    }

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

    // Render Passes
    {
        m_renderPasses.push_back(std::make_unique<ForwardPass>());
        m_renderPasses.push_back(std::make_unique<ImGuiPass>());
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

    int i = 0;
    for (auto& pass : m_renderPasses)
    {
        RenderTarget colorAttachment = {m_swapchain.m_images[m_imageIndex],
                                        m_swapchain.m_imageViews[m_imageIndex]};
        colorAttachment.ClearValue = {0.0f, 0.0f, 0.0f, 1.0f};
        VkFormat depthFormat = nijiEngine.m_context.find_depth_format();
        RenderTarget depthStencil = {m_swapchain.m_depthImage, m_swapchain.m_depthImageView,
                                     depthFormat};
        depthStencil.ClearValue = {1.0f, 0.0f};

        RenderInfo info = {m_swapchain.m_extent};
        info.ColorAttachment = colorAttachment;
        info.DepthAttachment = depthStencil;
        info.HasDepth = true;
        // If we are on the last pass, tell the pass to prepare for present
        info.PrepareForPresent = i == m_renderPasses.size() - 1 ? true : false;

        pass->record(*this, cmd, info);

        i++;
    }

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

    /*auto view = nijiEngine.ecs.m_registry.view<Transform, MeshComponent>();
    for (auto&& [entity, trans, mesh] : view.each())
    {
        auto& model = mesh.Model;
        auto& modelMesh = model->m_meshes[mesh.MeshID];
        auto& modelMaterial = model->m_materials[mesh.MeshID];
        model->cleanup();
    }*/

    m_fallbackTexture.cleanup();

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

    auto& cameraSystem = nijiEngine.ecs.find_system<CameraSystem>();
    auto& camera = cameraSystem.m_camera;

    UniformBufferObject ubo = {};

    ubo.View = camera.GetViewMatrix();
    ubo.Proj = camera.GetProjectionMatrix(); /*glm::perspective(glm::radians(45.0f),
                                m_swapchain.m_extent.width / (float)m_swapchain.m_extent.height,
                                0.1f, 10.0f);*/
        
    ubo.Proj[1][1] *= -1;

    memcpy(m_ubos[currentImage].Data, &ubo, sizeof(ubo));
}