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

#include "passes/line_render_pass.hpp"
#include "passes/light_culling.hpp"
#include "passes/forward_pass.hpp"
#include "passes/skybox_pass.hpp"
#include "passes/render_pass.hpp"
#include "passes/depth_pass.hpp"
#include "passes/imgui_pass.hpp"

#include "model/model.hpp"

#include "swapchain.hpp"
#include "engine.hpp"
#include <iostream>

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
        desc.IsMipMapped = true;
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

    int width = 0, height = 0;
    nijiEngine.m_context.get_window_size(width, height);

    uint32_t totalThreadsX = ceil((float)width / GROUP_SIZE);
    uint32_t totalThreadsY = ceil((float)height / GROUP_SIZE);
    glm::u32vec2 totalThreads = {totalThreadsX, totalThreadsY};
    // Create Light Grid RWTexture
    {

        TextureDesc desc = {};
        desc.Width = totalThreads.x;
        desc.Height = totalThreads.y;
        desc.Channels = 2;
        desc.IsMipMapped = false;
        desc.Data = nullptr;
        desc.Format = VK_FORMAT_R32G32_UINT;
        desc.MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
        desc.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        desc.IsReadWrite = true;
        m_lightGridTexture = Texture(desc);
    }

    // Create Light Index List Buffer
    {
        VkDeviceSize bufferSize = sizeof(LightIndexList);
        m_lightIndexList.resize(MAX_FRAMES_IN_FLIGHT);
        const uint32_t totalTiles = totalThreads.x * totalThreads.y;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            LightIndexList buffer = {};
            buffer.counter = 0;
            BufferDesc bufferDesc = {};
            bufferDesc.IsPersistent = true;
            bufferDesc.Name = "Light Index List Buffer";
            bufferDesc.Size = sizeof(LightIndexList) * totalTiles * MAX_LIGHTS_PER_TILE;
            bufferDesc.Usage = BufferDesc::BufferUsage::Storage;
            m_lightIndexList[i] = Buffer(bufferDesc, &buffer);
        }
    }

    // Global Descriptor
    {
        // Camera Data
        {
            m_cameraData.resize(MAX_FRAMES_IN_FLIGHT);
            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            {
                CameraData ubo = {};
                BufferDesc bufferDesc = {};
                bufferDesc.IsPersistent = true;
                bufferDesc.Name = "Camera UBO";
                bufferDesc.Size = sizeof(CameraData);
                bufferDesc.Usage = BufferDesc::BufferUsage::Uniform;
                m_cameraData[i] = Buffer(bufferDesc, &ubo);
            }
        }

        // Point Lights (Sphere Data Only)
        {
            // Create Point Light Buffer
            {
                VkDeviceSize bufferSize = sizeof(Sphere);
                m_spheres.resize(MAX_FRAMES_IN_FLIGHT);
                for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
                {
                    std::array<Sphere, MAX_POINT_LIGHTS> buffer = {};
                    BufferDesc bufferDesc = {};
                    bufferDesc.IsPersistent = true;
                    bufferDesc.Name = "Point Lights (Sphere) Data";
                    bufferDesc.Size = sizeof(Sphere) * MAX_POINT_LIGHTS;
                    bufferDesc.Usage = BufferDesc::BufferUsage::Storage;
                    m_spheres[i] = Buffer(bufferDesc, nullptr);
                }
            }
        }

        // Create Scene Info Buffer
        {
            VkDeviceSize bufferSize = sizeof(SceneInfo);
            m_sceneInfoBuffer.resize(MAX_FRAMES_IN_FLIGHT);
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                SceneInfo ubo = {};
                BufferDesc bufferDesc = {};
                bufferDesc.IsPersistent = true;
                bufferDesc.Name = "Scene Info Data";
                bufferDesc.Size = sizeof(SceneInfo);
                bufferDesc.Usage = BufferDesc::BufferUsage::Uniform;
                m_sceneInfoBuffer[i] = Buffer(bufferDesc, &ubo);
            }
        }

        DescriptorInfo info = {};
        DescriptorBinding binding = {};
        binding.Type = DescriptorBinding::BindType::UBO;
        binding.Count = 1;
        binding.Stage = DescriptorBinding::BindStage::ALL_GRAPHICS;
        binding.Sampler = nullptr;
        binding.Resource = &m_cameraData;
        info.Bindings.push_back(binding);
        info.Name = "Global Descriptor";

        m_globalDescriptor = Descriptor(info);
    }

    // Render Passes
    {
        m_renderPasses.push_back(std::make_unique<SkyboxPass>());
        m_renderPasses.push_back(std::make_unique<DepthPass>());
        m_renderPasses.push_back(std::make_unique<LightCullingPass>());
        m_renderPasses.push_back(std::make_unique<ForwardPass>());
        m_renderPasses.push_back(std::make_unique<LineRenderPass>());
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
    for (int i = 0; i < m_commandBuffers.size(); i++)
    {
        std::string name = "Command Buffer ";
        name = name + std::to_string(i);
        m_commandBuffers[i].m_name = const_cast<char*>(name.c_str());
    }

    create_sync_objects();

    m_colorAttachments[0] = {m_swapchain.m_images[0], m_swapchain.m_imageViews[0]};
    m_colorAttachments[0].ClearValue = {0.1f, 0.1f, 0.1f, 1.0f};
    m_colorAttachments[0].Name = "Color Target 0";
    m_colorAttachments[1] = {m_swapchain.m_images[1], m_swapchain.m_imageViews[1]};
    m_colorAttachments[1].ClearValue = {0.1f, 0.1f, 0.1f, 1.0f};
    m_colorAttachments[1].Name = "Color Target 1";

    VkFormat depthFormat = nijiEngine.m_context.find_depth_format();
    m_depthAttachment = {m_swapchain.m_depthImage, m_swapchain.m_depthImageView, depthFormat};
    m_depthAttachment.ClearValue = {1.0f, 0.0f};
    m_depthAttachment.Name = "Depth Target";

    m_renderInfo = RenderInfo(m_swapchain.m_extent);
    m_renderInfo.ColorAttachment = &m_colorAttachments[0];
    m_renderInfo.DepthAttachment = &m_depthAttachment;
}

void Renderer::update(const float dt)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    VkFence frameFence = m_inFlightFences[m_currentFrame];
    vkWaitForFences(m_context->m_device, 1, &frameFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_context->m_device, 1, &frameFence);

    auto& cmd = m_commandBuffers[m_currentFrame];
    cmd.begin_list("Frame Commmand Buffer");

    update_uniform_buffer(m_currentFrame);

    for (auto& pass : m_renderPasses)
    {
        pass->update(*this, cmd);
    }
}

void Renderer::render()
{
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

    m_colorAttachments[m_imageIndex].Image = m_swapchain.m_images[m_imageIndex];
    m_colorAttachments[m_imageIndex].ImageView = m_swapchain.m_imageViews[m_imageIndex];

    VkFormat depthFormat = nijiEngine.m_context.find_depth_format();
    m_depthAttachment.Image = m_swapchain.m_depthImage;
    m_depthAttachment.ImageView = m_swapchain.m_depthImageView;
    m_depthAttachment.ClearValue = {1.0f, 0.0f};

    m_renderInfo.ColorAttachment = &m_colorAttachments[m_imageIndex];
    m_renderInfo.HasDepth = true;

    int i = 0;
    for (auto& pass : m_renderPasses)
    {
        // If we are on the last pass, tell the pass to prepare for present
        m_renderInfo.PrepareForPresent = i == m_renderPasses.size() - 1 ? true : false;

        pass->record(*this, cmd, m_renderInfo);

        i++;
    }

    cmd.end_list();

    VkSemaphore submitSemaphore = m_renderFinishedSemaphores[m_imageIndex];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_NONE};
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

    m_lightGridTexture.cleanup();

    for (int i = 0; i < m_lightIndexList.size(); i++)
    {
        m_lightIndexList[i].cleanup();
    }

    for (int i = 0; i < m_cameraData.size(); i++)
    {
        m_cameraData[i].cleanup();
    }

    for (int i = 0; i < m_spheres.size(); i++)
    {
        m_spheres[i].cleanup();
    }

    for (int i = 0; i < m_sceneInfoBuffer.size(); i++)
    {
        m_sceneInfoBuffer[i].cleanup();
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

        std::string imageAvailableName = "Image Available Semaphore ";
        imageAvailableName += std::to_string(i);
        SetObjectName(m_context->m_device, VK_OBJECT_TYPE_SEMAPHORE, m_imageAvailableSemaphores[i],
                      imageAvailableName.c_str());

        std::string renderFinishedName = "Render Finished Semaphore ";
        renderFinishedName += std::to_string(i);
        SetObjectName(m_context->m_device, VK_OBJECT_TYPE_SEMAPHORE, m_renderFinishedSemaphores[i],
                      renderFinishedName.c_str());

        std::string inFlightFenceName = "In-Flight Fence ";
        inFlightFenceName += std::to_string(i);
        SetObjectName(m_context->m_device, VK_OBJECT_TYPE_FENCE, m_inFlightFences[i],
                      inFlightFenceName.c_str());
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
    {
        CameraData ubo = {};

        ubo.View = camera.GetViewMatrix();
        ubo.Proj = camera.GetProjectionMatrix();
        ubo.Pos = camera.Position;

        ubo.Proj[1][1] *= -1;

        vkCmdUpdateBuffer(m_commandBuffers[currentImage].m_commandBuffer,
                          m_cameraData[currentImage].Handle, 0, sizeof(ubo), &ubo);
    }

    {
        std::vector<Sphere> pointLightsArray = {};
        {
            auto pointLightView = nijiEngine.ecs.m_registry.view<PointLight>();
            for (const auto& [ent, pointLight] : pointLightView.each())
            {
                if (pointLightsArray.size() < MAX_POINT_LIGHTS)
                {
                    Sphere s = {};
                    glm::vec4 centerWS = glm::vec4(pointLight.Position, 1.0f);
                    s.Center = glm::vec3(camera.GetViewMatrix() * centerWS);
                    //printf("World Space Pos: %f, %f, %f \n", centerWS.x, centerWS.y, centerWS.z);
                    //printf("View Space Pos: %f, %f, %f \n", s.Center.x, s.Center.y, s.Center.z);
                    s.Radius = pointLight.Range;
                    pointLightsArray.push_back(s);
                }
            }
            vkCmdUpdateBuffer(m_commandBuffers[currentImage].m_commandBuffer,
                              m_spheres[currentImage].Handle, 0,
                              sizeof(Sphere) * pointLightsArray.size(), pointLightsArray.data());

            //size_t dataSize = sizeof(Sphere) * pointLightsArray.size();

            //void* data = nullptr;
            //VkResult result = vmaMapMemory(m_context->m_allocator,
            //                               m_spheres[currentImage].BufferAllocation, &data);
            //if (result != VK_SUCCESS)
            //{
            //    throw std::runtime_error("Failed to map memory for spheres buffer.");
            //}

            //memcpy(data, pointLightsArray.data(), dataSize);

            ////// If not using VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, flush manually
            //// VkMappedMemoryRange range{};
            //// range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            //// range.memory = m_spheres[currentImage].Memory;
            //// range.offset = 0;
            //// range.size = dataSize;

            //// vkFlushMappedMemoryRanges(m_context->m_device, 1, &range);

            //// Unmap if buffer is not persistently mapped (optional)
            //vmaUnmapMemory(m_context->m_allocator, m_spheres[currentImage].BufferAllocation);
        }

        auto dirLightView = nijiEngine.ecs.m_registry.view<DirectionalLight>();
        for (const auto& [ent, dirLight] : dirLightView.each())
        {
            SceneInfo sceneInfo = {};
            sceneInfo.DirLight = dirLight;
            sceneInfo.PointLightCount = pointLightsArray.size();

            vkCmdUpdateBuffer(m_commandBuffers[currentImage].m_commandBuffer,
                              m_sceneInfoBuffer[currentImage].Handle, 0, sizeof(SceneInfo),
                              &sceneInfo);
        }
    }
}