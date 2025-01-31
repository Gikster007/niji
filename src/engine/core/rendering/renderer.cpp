#include "renderer.hpp"

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

using namespace niji;

Renderer::Renderer(Context& context)
{
    m_context = &context;

    //// Quad 1
    //m_vertices.push_back({{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});
    //m_vertices.push_back({{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
    //m_vertices.push_back({{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});
    //m_vertices.push_back({{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}});
    //m_indices.push_back(0);
    //m_indices.push_back(1);
    //m_indices.push_back(2);
    //m_indices.push_back(2);
    //m_indices.push_back(3);
    //m_indices.push_back(0);
    //// Quad 2
    //m_vertices.push_back({{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});
    //m_vertices.push_back({{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
    //m_vertices.push_back({{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});
    //m_vertices.push_back({{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}});
    //m_indices.push_back(4);
    //m_indices.push_back(5);
    //m_indices.push_back(6);
    //m_indices.push_back(6);
    //m_indices.push_back(7);
    //m_indices.push_back(4);
    //  Front face
    m_vertices.push_back({{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});
    m_vertices.push_back({{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
    m_vertices.push_back({{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});
    m_vertices.push_back({{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}});
    m_indices.push_back(0);
    m_indices.push_back(1);
    m_indices.push_back(2);
    m_indices.push_back(2);
    m_indices.push_back(3);
    m_indices.push_back(0);

    // Back face
    m_vertices.push_back({{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});
    m_vertices.push_back({{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
    m_vertices.push_back({{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});
    m_vertices.push_back({{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}});
    m_indices.push_back(4);
    m_indices.push_back(5);
    m_indices.push_back(6);
    m_indices.push_back(6);
    m_indices.push_back(7);
    m_indices.push_back(4);

    // Left face
    m_vertices.push_back({{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});
    m_vertices.push_back({{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
    m_vertices.push_back({{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});
    m_vertices.push_back({{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}});
    m_indices.push_back(8);
    m_indices.push_back(9);
    m_indices.push_back(10);
    m_indices.push_back(10);
    m_indices.push_back(11);
    m_indices.push_back(8);

    // Right face
    m_vertices.push_back({{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});
    m_vertices.push_back({{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
    m_vertices.push_back({{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});
    m_vertices.push_back({{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}});
    m_indices.push_back(12);
    m_indices.push_back(13);
    m_indices.push_back(14);
    m_indices.push_back(14);
    m_indices.push_back(15);
    m_indices.push_back(12);

    // Top face
    m_vertices.push_back({{-0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});
    m_vertices.push_back({{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
    m_vertices.push_back({{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});
    m_vertices.push_back({{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}});
    m_indices.push_back(16);
    m_indices.push_back(17);
    m_indices.push_back(18);
    m_indices.push_back(18);
    m_indices.push_back(19);
    m_indices.push_back(16);

    // Bottom face
    m_vertices.push_back({{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});
    m_vertices.push_back({{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
    m_vertices.push_back({{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});
    m_vertices.push_back({{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}});
    m_indices.push_back(20);
    m_indices.push_back(21);
    m_indices.push_back(22);
    m_indices.push_back(22);
    m_indices.push_back(23);
    m_indices.push_back(20);

    init();
}

Renderer::~Renderer()
{
    cleanup();
}

void Renderer::init()
{
    create_swap_chain();
    create_image_views();
    create_descriptor_set_layout();
    create_graphics_pipeline();

    create_depth_resources();

    create_texture_iamge();
    create_texture_image_view();
    create_texture_sampler();

    create_vertex_buffer();
    create_index_buffer();
    create_uniform_buffers();

    create_descriptor_pool();
    create_descriptor_sets();

    create_command_buffers();
    create_sync_objects();
}

void Renderer::update(const float dt)
{
    
}

void Renderer::render()
{
    vkWaitForFences(m_context->m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE,
                    UINT64_MAX);

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

    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
    record_command_buffer(m_commandBuffers[m_currentFrame], imageIndex);

    update_uniform_buffer(m_currentFrame);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait_semaphores;
    submitInfo.pWaitDstStageMask = wait_stages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
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

    vkDestroySampler(m_context->m_device, m_textureSampler, nullptr);

    vkDestroyImageView(m_context->m_device, m_textureImageView, nullptr);

    vkDestroyImage(m_context->m_device, m_textureImage, nullptr);
    vkFreeMemory(m_context->m_device, m_textureImageMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(m_context->m_device, m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_context->m_device, m_uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(m_context->m_device, m_descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(m_context->m_device, m_descriptorSetLayout, nullptr);

    vkDestroyBuffer(m_context->m_device, m_indexBuffer, nullptr);
    vkFreeMemory(m_context->m_device, m_indexBufferMemory, nullptr);

    vkDestroyBuffer(m_context->m_device, m_vertexBuffer, nullptr);
    vkFreeMemory(m_context->m_device, m_vertexBufferMemory, nullptr);

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
    SwapChainSupportDetails swapChainSupport = SwapChainSupportDetails::query_swap_chain_support(
        m_context->m_physicalDevice, m_context->m_surface);

    VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.Formats);
    VkPresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.PresentModes);
    VkExtent2D extent = choose_swap_extent(swapChainSupport.Capabilities);

    uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
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
    uint32_t queueFamilyIndices[] = {indices.GraphicsFamily.value(),
                                       indices.PresentFamily.value()};

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

    if (vkCreateSwapchainKHR(m_context->m_device, &createInfo, nullptr, &m_swapChain) !=
        VK_SUCCESS)
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
        m_swapChainImageViews[i] = create_image_view(
            m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Renderer::cleanup_swap_chain()
{
    vkDestroyImageView(m_context->m_device, m_depthImageView, nullptr);
    vkDestroyImage(m_context->m_device, m_depthImage, nullptr);
    vkFreeMemory(m_context->m_device, m_depthImageMemory, nullptr);

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

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                       fragShaderStageInfo};

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
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
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
        throw std::runtime_error("Failed to Open File!");

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

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_context->m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

    if (vkAllocateCommandBuffers(m_context->m_device, &allocInfo, m_commandBuffers.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Allocate Command Buffers!");
}

void Renderer::record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to Begin Recording Command Buffer!");

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderingAttachmentInfoKHR colorAttachmentInfo = {};
    colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachmentInfo.imageView = m_swapChainImageViews[imageIndex];
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentInfo.clearValue = clearColor;
    VkRenderingAttachmentInfoKHR depthAttachmentInfo = {};
    depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachmentInfo.imageView = m_depthImageView;
    depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentInfo.clearValue = {1.0f, 0.0f};
    VkRenderingInfoKHR renderInfo = {};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderInfo.renderArea.offset = {0, 0};
    renderInfo.renderArea.extent = m_swapChainExtent;
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachmentInfo;
    renderInfo.pDepthAttachment = &depthAttachmentInfo;

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageMemoryBarrier.image = m_swapChainImages[imageIndex];
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr,
                         1, &imageMemoryBarrier);

    VkImageMemoryBarrier depthMemoryBarrier = {};
    depthMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    depthMemoryBarrier.srcAccessMask = 0;
    depthMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depthMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthMemoryBarrier.image = m_depthImage;
    VkImageSubresourceRange depthSubresourceRange = {};
    depthSubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthSubresourceRange.baseMipLevel = 0;
    depthSubresourceRange.levelCount = 1;
    depthSubresourceRange.baseArrayLayer = 0;
    depthSubresourceRange.layerCount = 1;
    VkFormat depth_format = m_context->find_depth_format();
    if (m_context->has_stencil_component(depth_format))
    {
        depthSubresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    depthMemoryBarrier.subresourceRange = depthSubresourceRange;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &depthMemoryBarrier);

    m_context->BeginRendering(commandBuffer, &renderInfo); // vkCmdBeginRenderingKHR

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChainExtent.width);
    viewport.height = static_cast<float>(m_swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0,
                            1, &m_descriptorSets[m_currentFrame], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

    m_context->EndRendering(commandBuffer); // vkCmdEndRenderingKHR

    VkImageMemoryBarrier imMemBarrier = {};
    imMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imMemBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imMemBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imMemBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imMemBarrier.image = m_swapChainImages[imageIndex];
    imMemBarrier.subresourceRange = subresourceRange;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imMemBarrier);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to Record Command Buffer!");
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

void Renderer::create_vertex_buffer()
{
    VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

    VkBuffer stagingBuffer = {};
    VkDeviceMemory stagingBufferMemory = {};
    create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    void* data = nullptr;
    vkMapMemory(m_context->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);

    memcpy(data, m_vertices.data(), (size_t)bufferSize);

    vkUnmapMemory(m_context->m_device, stagingBufferMemory);

    create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

    copy_buffer(stagingBuffer, m_vertexBuffer, bufferSize);

    vkDestroyBuffer(m_context->m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_context->m_device, stagingBufferMemory, nullptr);
}

void Renderer::create_index_buffer()
{
    VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

    VkBuffer stagingBuffer = {};
    VkDeviceMemory stagingBufferMemory = {};
    create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    void* data = nullptr;
    vkMapMemory(m_context->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_indices.data(), (size_t)bufferSize);
    vkUnmapMemory(m_context->m_device, stagingBufferMemory);

    create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

    copy_buffer(stagingBuffer, m_indexBuffer, bufferSize);

    vkDestroyBuffer(m_context->m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_context->m_device, stagingBufferMemory, nullptr);
}

void Renderer::create_uniform_buffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        create_buffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      m_uniformBuffers[i], m_uniformBuffersMemory[i]);

        vkMapMemory(m_context->m_device, m_uniformBuffersMemory[i], 0, bufferSize, 0,
                    &m_uniformBuffersMapped[i]);
    }
}

void Renderer::update_uniform_buffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime)
            .count();

    UniformBufferObject ubo = {};
    ubo.Model =
        glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.Proj = glm::perspective(glm::radians(45.0f),
                                m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.1f,
                                10.0f);
    ubo.Proj[1][1] *= -1;

    memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Renderer::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags properties, VkBuffer& buffer,
                             VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_context->m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Buffer!");

    VkMemoryRequirements memRequirements = {};
    vkGetBufferMemoryRequirements(m_context->m_device, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_context->find_memory_type(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_context->m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to Allocate Vertex Buffer Memory!");

    vkBindBufferMemory(m_context->m_device, buffer, bufferMemory, 0);
}

void Renderer::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    end_single_time_commands(commandBuffer);
}

void Renderer::create_texture_iamge()
{
    int width = -1, height = -1, channels = -1;
    stbi_uc* pixels = stbi_load("textures/photon-the-final-gathering.jpg", &width, &height,
                                &channels, STBI_rgb_alpha);
    VkDeviceSize imageSize = width * height * 4;
    if (!pixels)
        throw std::runtime_error("Failed to Load Texture Image!");

    VkBuffer stagingBuffer = {};
    VkDeviceMemory stagingBufferMemory = {};

    create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    void* data = nullptr;
    vkMapMemory(m_context->m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_context->m_device, stagingBufferMemory);

    stbi_image_free(pixels);

    create_image(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory);

    transition_image_layout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(stagingBuffer, m_textureImage, static_cast<uint32_t>(width),
                         static_cast<uint32_t>(height));
    transition_image_layout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_context->m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_context->m_device, stagingBufferMemory, nullptr);
}

void Renderer::create_texture_image_view()
{
    m_textureImageView =
        create_image_view(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Renderer::create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                            VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                            VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = static_cast<uint32_t>(width);
    image_info.extent.height = static_cast<uint32_t>(height);
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = 0;

    if (vkCreateImage(m_context->m_device, &image_info, nullptr, &image) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Image!");

    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(m_context->m_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        m_context->find_memory_type(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_context->m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to Allocate Image Memory!");

    vkBindImageMemory(m_context->m_device, image, imageMemory, 0);
}

VkImageView Renderer::create_image_view(VkImage image, VkFormat format,
                                        VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView = {};
    if (vkCreateImageView(m_context->m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void Renderer::transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                       VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkPipelineStageFlags sourceStage = {};
    VkPipelineStageFlags destinationStage = {};

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (m_context->has_stencil_component(format))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
        throw std::invalid_argument("Unsupported Layout Transition");

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr,
                         1, &barrier);

    end_single_time_commands(commandBuffer);
}

void Renderer::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &region);

    end_single_time_commands(commandBuffer);
}

void Renderer::create_texture_sampler()
{
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(m_context->m_physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_context->m_device, &samplerInfo, nullptr, &m_textureSampler) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Create Texture Sampler!");
}

void Renderer::create_depth_resources()
{
    VkFormat depthFormat = m_context->find_depth_format();
    create_image(m_swapChainExtent.width, m_swapChainExtent.height, depthFormat,
                 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);
    m_depthImageView = create_image_view(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    transition_image_layout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkCommandBuffer Renderer::begin_single_time_commands()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_context->m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = {};
    vkAllocateCommandBuffers(m_context->m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void Renderer::end_single_time_commands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_context->m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_context->m_graphicsQueue);

    vkFreeCommandBuffers(m_context->m_device, m_context->m_commandPool, 1, &commandBuffer);
}

void Renderer::create_descriptor_pool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(m_context->m_device, &poolInfo, nullptr, &m_descriptorPool) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Create Descriptor Pool!");
}

void Renderer::create_descriptor_sets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(m_context->m_device, &allocInfo, m_descriptorSets.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Allocate Descriptor Sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler = m_textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pTexelBufferView = nullptr;
        descriptorWrites[0].pImageInfo = nullptr;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = nullptr;
        descriptorWrites[1].pTexelBufferView = nullptr;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_context->m_device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

void Renderer::create_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,
                                                            samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_context->m_device, &layoutInfo, nullptr,
                                    &m_descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Descriptor Set Layout");
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
