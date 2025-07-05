#include "renderer.hpp"

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vk_mem_alloc.h>

using namespace niji;

#include "../core/vulkan-functions.hpp"
#include <engine.hpp>
#include <core/components/render-components.hpp>
#include <core/components/transform.hpp>

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
    create_swap_chain();
    create_image_views();
    create_graphics_pipeline();

    create_depth_resources();

    create_uniform_buffers();

    create_command_buffers();
    create_sync_objects();
}

void Renderer::update(const float dt)
{
}

void Renderer::render()
{
    vkWaitForFences(m_context->m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    // Get index of the next available presentable image
    uint32_t imageIndex = UINT64_MAX; // init to high value to avoid confusion
    VkResult result = vkAcquireNextImageKHR(m_context->m_device, m_swapChain, UINT64_MAX,
                                            m_imageAvailableSemaphores[m_currentFrame],
                                            VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreate_swap_chain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to Acquire Swap Chain Image!");
    }

    vkResetFences(m_context->m_device, 1, &m_inFlightFences[m_currentFrame]);

    auto& cmd = m_commandBuffers[m_currentFrame];
    cmd.reset();

    cmd.begin_list("Forward Pass");

    RenderTarget colorAttachment = {m_swapChainImages[imageIndex],
                                    m_swapChainImageViews[imageIndex]};
    colorAttachment.ClearValue = {0.0f, 0.0f, 0.0f, 1.0f};
    VkFormat depthFormat = m_context->find_depth_format();
    RenderTarget depthStencil = {m_depthImage, m_depthImageView, depthFormat};
    depthStencil.ClearValue = {1.0f, 0.0f};

    RenderInfo info = {m_swapChainExtent};
    info.ColorAttachments.push_back(colorAttachment);
    info.DepthAttachment = depthStencil;
    info.HasDepth = true;

    cmd.begin_rendering(info);

    cmd.bind_pipeline(m_graphicsPipeline);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChainExtent.width);
    viewport.height = static_cast<float>(m_swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    cmd.bind_viewport(viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;
    cmd.bind_scissor(scissor);

    auto view = nijiEngine.ecs.m_registry.view<Transform, MeshComponent>();
    for (auto&& [entity, trans, mesh] : view.each())
    {
        auto& model = mesh.Model;
        auto& modelMesh = model->m_meshes[mesh.MeshID];
        auto& modelMaterial = model->m_materials[mesh.MeshID];

        VkBuffer vertexBuffers[] = {modelMesh.m_vertexBuffer.Handle};
        VkDeviceSize offsets[] = {0};
        cmd.bind_vertex_buffer(0, 1, vertexBuffers, offsets);
        cmd.bind_index_buffer(modelMesh.m_indexBuffer.Handle, 0,
                            modelMesh.m_ushortIndices ? VK_INDEX_TYPE_UINT16
                                                      : VK_INDEX_TYPE_UINT32);

        cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                               &modelMaterial.m_descriptorSets[m_currentFrame], 0, nullptr);

        cmd.draw_indexed(static_cast<uint32_t>(modelMesh.m_indexCount), 1, 0, 0, 0);
    }

    cmd.end_rendering(info);
    cmd.end_list();

    update_uniform_buffer(m_currentFrame);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait_semaphores;
    submitInfo.pWaitDstStageMask = wait_stages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame].m_commandBuffer;
    VkSemaphore signal_semaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(m_context->m_graphicsQueue, 1, &submitInfo,
                      m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to Submit Draw Command Buffer!");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapChains[] = {m_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_context->m_presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        m_context->m_framebufferResized)
    {
        recreate_swap_chain();
        m_context->m_framebufferResized = false;
    }
    else if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to Present Swap Chain Image!");

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::cleanup()
{
    cleanup_swap_chain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // Make sure the buffer is unmapped before destroying it
        if (m_uniformBuffersAllocations[i] != VK_NULL_HANDLE)
        {
            vmaUnmapMemory(m_context->m_allocator, m_uniformBuffersAllocations[i]);
        }

        vmaDestroyBuffer(m_context->m_allocator, m_uniformBuffers[i],
                         m_uniformBuffersAllocations[i]);
    }

    auto view = nijiEngine.ecs.m_registry.view<Transform, MeshComponent>();
    for (auto&& [entity, trans, mesh] : view.each())
    {
        auto& model = mesh.Model;
        auto& modelMesh = model->m_meshes[mesh.MeshID];
        auto& modelMaterial = model->m_materials[mesh.MeshID];
        model->cleanup();
        modelMaterial.cleanup();
        modelMesh.cleanup();
    }

    vkDestroyPipeline(m_context->m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_context->m_device, m_pipelineLayout, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_context->m_device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_context->m_device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_context->m_device, m_inFlightFences[i], nullptr);
    }
}

VkSurfaceFormatKHR Renderer::choose_swap_surface_format(
    const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }
    return availableFormats[0];
}

VkPresentModeKHR Renderer::choose_swap_present_mode(
    const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width = -1, height = -1;
        glfwGetFramebufferSize(m_context->m_window, &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

void Renderer::create_swap_chain()
{
    SwapChainSupportDetails swapChainSupport =
        SwapChainSupportDetails::query_swap_chain_support(m_context->m_physicalDevice,
                                                          m_context->m_surface);

    VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.Formats);
    VkPresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.PresentModes);
    VkExtent2D extent = choose_swap_extent(swapChainSupport.Capabilities);

    uint32_t imageCount = swapChainSupport.Capabilities.minImageCount;
    if (swapChainSupport.Capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.Capabilities.maxImageCount)
        imageCount = swapChainSupport.Capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_context->m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices =
        QueueFamilyIndices::find_queue_families(m_context->m_physicalDevice, m_context->m_surface);
    uint32_t queueFamilyIndices[] = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

    if (indices.GraphicsFamily != indices.PresentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_context->m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Swap Chain!");

    vkGetSwapchainImagesKHR(m_context->m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_context->m_device, m_swapChain, &imageCount,
                            m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void Renderer::recreate_swap_chain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_context->m_window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_context->m_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_context->m_device);

    cleanup_swap_chain();

    create_swap_chain();
    create_image_views();
    create_depth_resources();
}

void Renderer::create_image_views()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());
    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        m_swapChainImageViews[i] = m_context->create_image_view(m_swapChainImages[i], m_swapChainImageFormat,
                                                     VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Renderer::cleanup_swap_chain()
{
    vkDestroyImageView(m_context->m_device, m_depthImageView, nullptr);
    vmaDestroyImage(m_context->m_allocator, m_depthImage, m_depthImageAllocation);

    for (auto imageView : m_swapChainImageViews)
    {
        vkDestroyImageView(m_context->m_device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_context->m_device, m_swapChain, nullptr);
}

void Renderer::create_graphics_pipeline()
{
    auto vertShaderCode = read_file("shaders/vert.spv");
    auto fragShaderCode = read_file("shaders/frag.spv");

    VkShaderModule vertShaderModule = create_shader_module(vertShaderCode);
    VkShaderModule fragShaderModule = create_shader_module(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    auto bindingDescription = Vertex::get_binding_description();
    auto attributeDescription = Vertex::get_attribute_description();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescription.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swapChainExtent.width;
    viewport.height = (float)m_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    auto view = nijiEngine.ecs.m_registry.view<Transform, MeshComponent>();
    for (auto&& [entity, trans, mesh] : view.each())
    {
        auto& model = mesh.Model;
        auto& modelMaterial = model->m_materials[mesh.MeshID];

        pipelineLayoutInfo.pSetLayouts = &modelMaterial.m_descriptorSetLayout;
    }
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(m_context->m_device, &pipelineLayoutInfo, nullptr,
                               &m_pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Pipeline Layout!");

    VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &m_swapChainImageFormat;
    pipelineRenderingInfo.depthAttachmentFormat = m_context->find_depth_format();

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = nullptr /*m_render_pass*/;
    pipelineInfo.pNext = &pipelineRenderingInfo;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_context->m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                  &m_graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Graphics Pipeline!");

    vkDestroyShaderModule(m_context->m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_context->m_device, vertShaderModule, nullptr);
}

std::vector<char> Renderer::read_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        printf("Failed to open File!");
        throw std::runtime_error("Failed to Open File!");
    }

    size_t fileSize = (size_t)file.tellg();
    printf("File size of %s is %zd \n", filename.c_str(), fileSize);
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

VkShaderModule Renderer::create_shader_module(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = {};
    if (vkCreateShaderModule(m_context->m_device, &createInfo, nullptr, &shaderModule) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Create Shader Module!");

    return shaderModule;
}

void Renderer::create_command_buffers()
{
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
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

void Renderer::create_uniform_buffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_context->create_buffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
                      m_uniformBuffers[i], m_uniformBuffersAllocations[i], true);

        vmaMapMemory(m_context->m_allocator, m_uniformBuffersAllocations[i],
                     &m_uniformBuffersMapped[i]);
    }
}

void Renderer::update_uniform_buffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo = {};

    auto view = nijiEngine.ecs.m_registry.view<Transform, MeshComponent>();
    for (auto&& [entity, trans, mesh] : view.each())
    {
        auto& model = mesh.Model;
        auto& modelMesh = model->m_meshes[mesh.MeshID];

        ubo.Model =
            glm::rotate(trans.World(), time * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.Proj = glm::perspective(glm::radians(45.0f),
                                    m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.1f,
                                    10.0f);
        ubo.Proj[1][1] *= -1;

        memcpy(model->m_ubo.UniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }
}

void Renderer::create_depth_resources()
{
    VkFormat depthFormat = m_context->find_depth_format();
    m_context->create_image(m_swapChainExtent.width, m_swapChainExtent.height, depthFormat,
                 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                 VMA_MEMORY_USAGE_GPU_ONLY, m_depthImage, m_depthImageAllocation);
    m_depthImageView = m_context->create_image_view(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    m_context->transition_image_layout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkVertexInputBindingDescription Vertex::get_binding_description()
{
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::get_attribute_description()
{
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, Pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, Color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, TexCoord);

    return attributeDescriptions;
}
