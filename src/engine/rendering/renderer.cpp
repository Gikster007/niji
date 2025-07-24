#include "renderer.hpp"

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>

#include <vk_mem_alloc.h>

#include <slang/slang.h>

#include <imgui.h>
#include <stb_image.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

using namespace niji;

#include "../../app/camera_system.hpp"

#include "core/components/render-components.hpp"
#include "core/components/transform.hpp"
#include "core/vulkan-functions.hpp"

#include "passes/skybox_pass.hpp"

#include "model/model.hpp"

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

inline static void CreateCube(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices)
{
    vertices = {
        {-1.0f, 1.0f, -1.0f},  // 0
        {-1.0f, -1.0f, -1.0f}, // 1
        {1.0f, -1.0f, -1.0f},  // 2
        {1.0f, 1.0f, -1.0f},   // 3
        {-1.0f, 1.0f, 1.0f},   // 4
        {-1.0f, -1.0f, 1.0f},  // 5
        {1.0f, -1.0f, 1.0f},   // 6
        {1.0f, 1.0f, 1.0f}     // 7
    };

    indices = {
        0, 1, 2, 2, 3, 0, // Front face
        4, 5, 1, 1, 0, 4, // Left face
        7, 6, 5, 5, 4, 7, // Back face
        3, 2, 6, 6, 7, 3, // Right face
        4, 0, 3, 3, 7, 4, // Top face
        1, 5, 6, 6, 2, 1  // Bottom face
    };
}

void Renderer::init()
{
    // Fallback Texture
    {
        int width = -1, height = -1, channels = -1;
        unsigned char* imageData = nullptr;

        imageData = stbi_load("assets/missing.png", &width, &height, &channels, STBI_rgb_alpha);

        if (!imageData)
        {
            printf("[Renderer] Failed to load Fallback Texture");
        }

        TextureDesc desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.Channels = 4;
        desc.Data = imageData;
        desc.Format = VK_FORMAT_R8G8B8A8_SRGB;
        desc.MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
        desc.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        m_fallbackTexture = Texture(desc);
    }

    // Create Cube
    {
        std::vector<glm::vec3> vertices = {};
        std::vector<uint32_t> indices = {};
        CreateCube(vertices, indices);

        m_cube = Mesh(vertices, indices);
    }

    // Global Descriptor
    {
        m_ubos.resize(MAX_FRAMES_IN_FLIGHT);
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            CameraData ubo = {};
            BufferDesc bufferDesc = {};
            bufferDesc.IsPersistent = true;
            bufferDesc.Name = "Camera UBO";
            bufferDesc.Size = sizeof(CameraData);
            bufferDesc.Usage = BufferDesc::BufferUsage::Uniform;
            m_ubos[i] = Buffer(bufferDesc, &ubo);
        }

        DescriptorInfo info = {};
        DescriptorBinding binding = {};
        binding.Type = DescriptorBinding::BindType::UBO;
        binding.Count = 1;
        binding.Stage = DescriptorBinding::BindStage::ALL_GRAPHICS;
        binding.Sampler = nullptr;
        binding.Resource = &m_ubos;
        info.Bindings.push_back(binding);

        m_globalDescriptor = Descriptor(info);
    }

    // Render Passes
    {
        m_renderPasses.push_back(std::make_unique<SkyboxPass>());
        m_renderPasses.push_back(std::make_unique<ForwardPass>());
        m_renderPasses.push_back(std::make_unique<ImGuiPass>());
    }
    {
        for (auto& pass : m_renderPasses)
        {
            pass->init(m_swapchain, m_globalDescriptor);
        }
    }

    // Create Command Buffers
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    create_sync_objects();
}

void Renderer::update(const float dt)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    update_uniform_buffer(m_currentFrame);

    for (auto& pass : m_renderPasses)
    {
        pass->update(*this);
    }
}

void Renderer::render()
{
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

    RenderTarget colorAttachment = {m_swapchain.m_images[m_imageIndex],
                                    m_swapchain.m_imageViews[m_imageIndex]};
    colorAttachment.ClearValue = {0.1f, 0.1f, 0.1f, 1.0f};
    VkFormat depthFormat = nijiEngine.m_context.find_depth_format();
    RenderTarget depthStencil = {m_swapchain.m_depthImage, m_swapchain.m_depthImageView,
                                 depthFormat};
    depthStencil.ClearValue = {1.0f, 0.0f};

    int i = 0;
    for (auto& pass : m_renderPasses)
    {
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

    m_cube.cleanup();

    m_globalDescriptor.cleanup();

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

    CameraData ubo = {};

    ubo.View = camera.GetViewMatrix();
    ubo.Proj = camera.GetProjectionMatrix();
    ubo.Pos = camera.Position;

    ubo.Proj[1][1] *= -1;

    memcpy(m_ubos[currentImage].Data, &ubo, sizeof(ubo));
}