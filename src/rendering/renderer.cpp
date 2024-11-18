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

void Renderer::draw()
{
    vkWaitForFences(m_context->m_device, 1, &m_in_flight_fences[current_frame], VK_TRUE,
                    UINT64_MAX);

    uint32_t image_index = UINT64_MAX; // init to high value to avoid confusion
    VkResult result = vkAcquireNextImageKHR(m_context->m_device, m_swap_chain, UINT64_MAX,
                                            m_image_available_semaphores[current_frame],
                                            VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreate_swap_chain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to Acquire Swap Chain Image!");
    }

    vkResetFences(m_context->m_device, 1, &m_in_flight_fences[current_frame]);

    vkResetCommandBuffer(m_command_buffers[current_frame], 0);
    record_command_buffer(m_command_buffers[current_frame], image_index);

    update_uniform_buffer(current_frame);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[] = {m_image_available_semaphores[current_frame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_command_buffers[current_frame];
    VkSemaphore signal_semaphores[] = {m_render_finished_semaphores[current_frame]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(m_context->m_graphics_queue, 1, &submit_info,
                      m_in_flight_fences[current_frame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to Submit Draw Command Buffer!");

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swap_chains[] = {m_swap_chain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    result = vkQueuePresentKHR(m_context->m_present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        m_context->m_framebuffer_resized)
        recreate_swap_chain();
    else if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to Present Swap Chain Image!");

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::cleanup()
{
    cleanup_swap_chain();

    vkDestroySampler(m_context->m_device, m_texture_sampler, nullptr);

    vkDestroyImageView(m_context->m_device, m_texture_image_view, nullptr);

    vkDestroyImage(m_context->m_device, m_texture_image, nullptr);
    vkFreeMemory(m_context->m_device, m_texture_image_memory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(m_context->m_device, m_uniform_buffers[i], nullptr);
        vkFreeMemory(m_context->m_device, m_uniform_buffers_memory[i], nullptr);
    }

    vkDestroyDescriptorPool(m_context->m_device, m_descriptor_pool, nullptr);

    vkDestroyDescriptorSetLayout(m_context->m_device, m_descriptor_set_layout, nullptr);

    vkDestroyBuffer(m_context->m_device, m_index_buffer, nullptr);
    vkFreeMemory(m_context->m_device, m_index_buffer_memory, nullptr);

    vkDestroyBuffer(m_context->m_device, m_vertex_buffer, nullptr);
    vkFreeMemory(m_context->m_device, m_vertex_buffer_memory, nullptr);

    vkDestroyPipeline(m_context->m_device, m_graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(m_context->m_device, m_pipeline_layout, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_context->m_device, m_render_finished_semaphores[i], nullptr);
        vkDestroySemaphore(m_context->m_device, m_image_available_semaphores[i], nullptr);
        vkDestroyFence(m_context->m_device, m_in_flight_fences[i], nullptr);
    }
}

VkSurfaceFormatKHR Renderer::choose_swap_surface_format(
    const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    for (const auto& available_format : available_formats)
    {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return available_format;
    }
    return available_formats[0];
}

VkPresentModeKHR Renderer::choose_swap_present_mode(
    const std::vector<VkPresentModeKHR>& available_present_modes)
{
    for (const auto& available_present_mode : available_present_modes)
    {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return available_present_mode;
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

        VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                                         capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                                          capabilities.maxImageExtent.height);
        return actual_extent;
    }
}

void Renderer::create_swap_chain()
{
    SwapChainSupportDetails swap_chain_support = SwapChainSupportDetails::query_swap_chain_support(
        m_context->m_physical_device, m_context->m_surface);

    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
    VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
    VkExtent2D extent = choose_swap_extent(swap_chain_support.capabilities);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount)
        image_count = swap_chain_support.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = m_context->m_surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices =
        QueueFamilyIndices::find_queue_families(m_context->m_physical_device, m_context->m_surface);
    uint32_t queue_family_indices[] = {indices.graphics_family.value(),
                                       indices.present_family.value()};

    if (indices.graphics_family != indices.present_family)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }
    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_context->m_device, &create_info, nullptr, &m_swap_chain) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Create Swap Chain!");

    vkGetSwapchainImagesKHR(m_context->m_device, m_swap_chain, &image_count, nullptr);
    m_swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(m_context->m_device, m_swap_chain, &image_count,
                            m_swap_chain_images.data());

    m_swap_chain_image_format = surface_format.format;
    m_swap_chain_extent = extent;
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
    m_swap_chain_image_views.resize(m_swap_chain_images.size());
    for (size_t i = 0; i < m_swap_chain_images.size(); i++)
    {
        m_swap_chain_image_views[i] = create_image_view(
            m_swap_chain_images[i], m_swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Renderer::cleanup_swap_chain()
{
    vkDestroyImageView(m_context->m_device, m_depth_image_view, nullptr);
    vkDestroyImage(m_context->m_device, m_depth_image, nullptr);
    vkFreeMemory(m_context->m_device, m_depth_image_memory, nullptr);

    for (auto image_view : m_swap_chain_image_views)
    {
        vkDestroyImageView(m_context->m_device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(m_context->m_device, m_swap_chain, nullptr);
}

void Renderer::create_graphics_pipeline()
{
    auto vert_shader_code = read_file("shaders/vert.spv");
    auto frag_shader_code = read_file("shaders/frag.spv");

    VkShaderModule vert_shader_module = create_shader_module(vert_shader_code);
    VkShaderModule frag_shader_module = create_shader_module(frag_shader_code);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info,
                                                       frag_shader_stage_info};

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    auto binding_description = Vertex::get_binding_description();
    auto attribute_description = Vertex::get_attribute_description();

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attribute_description.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_description.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swap_chain_extent.width;
    viewport.height = (float)m_swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swap_chain_extent;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

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

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &m_descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(m_context->m_device, &pipeline_layout_info, nullptr,
                               &m_pipeline_layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Pipeline Layout!");

    VkPipelineRenderingCreateInfoKHR pipeline_rendering_info = {};
    pipeline_rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &m_swap_chain_image_format;
    pipeline_rendering_info.depthAttachmentFormat = m_context->find_depth_format();

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front = {};
    depth_stencil.back = {};

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = m_pipeline_layout;
    pipeline_info.renderPass = nullptr /*m_render_pass*/;
    pipeline_info.pNext = &pipeline_rendering_info;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_context->m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                  &m_graphics_pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Graphics Pipeline!");

    vkDestroyShaderModule(m_context->m_device, frag_shader_module, nullptr);
    vkDestroyShaderModule(m_context->m_device, vert_shader_module, nullptr);
}

std::vector<char> Renderer::read_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("Failed to Open File!");

    size_t file_size = (size_t)file.tellg();
    printf("File size of %s is %zd \n", filename.c_str(), file_size);
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();
    return buffer;
}

VkShaderModule Renderer::create_shader_module(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader_module = {};
    if (vkCreateShaderModule(m_context->m_device, &create_info, nullptr, &shader_module) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Create Shader Module!");

    return shader_module;
}

void Renderer::create_command_buffers()
{
    m_command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = m_context->m_command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = (uint32_t)m_command_buffers.size();

    if (vkAllocateCommandBuffers(m_context->m_device, &alloc_info, m_command_buffers.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Allocate Command Buffers!");
}

void Renderer::record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("Failed to Begin Recording Command Buffer!");

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderingAttachmentInfoKHR color_attachment_info = {};
    color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    color_attachment_info.imageView = m_swap_chain_image_views[image_index];
    color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_info.clearValue = clear_color;
    VkRenderingAttachmentInfoKHR depth_attachment_info = {};
    depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth_attachment_info.imageView = m_depth_image_view;
    depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_info.clearValue = {1.0f, 0.0f};
    VkRenderingInfoKHR render_info = {};
    render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    render_info.renderArea.offset = {0, 0};
    render_info.renderArea.extent = m_swap_chain_extent;
    render_info.layerCount = 1;
    render_info.colorAttachmentCount = 1;
    render_info.pColorAttachments = &color_attachment_info;
    render_info.pDepthAttachment = &depth_attachment_info;

    VkImageMemoryBarrier image_memory_barrier = {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_memory_barrier.image = m_swap_chain_images[image_index];
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;
    image_memory_barrier.subresourceRange = subresource_range;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr,
                         1, &image_memory_barrier);

    VkImageMemoryBarrier depth_memory_barrier = {};
    depth_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    depth_memory_barrier.srcAccessMask = 0;
    depth_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depth_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_memory_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_memory_barrier.image = m_depth_image;
    VkImageSubresourceRange depth_subresource_range = {};
    depth_subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_subresource_range.baseMipLevel = 0;
    depth_subresource_range.levelCount = 1;
    depth_subresource_range.baseArrayLayer = 0;
    depth_subresource_range.layerCount = 1;
    VkFormat depth_format = m_context->find_depth_format();
    if (m_context->has_stencil_component(depth_format))
    {
        depth_subresource_range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    depth_memory_barrier.subresourceRange = depth_subresource_range;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &depth_memory_barrier);

    m_context->BeginRendering(command_buffer, &render_info); // vkCmdBeginRenderingKHR

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swap_chain_extent.width);
    viewport.height = static_cast<float>(m_swap_chain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swap_chain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkBuffer vertex_buffers[] = {m_vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, m_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0,
                            1, &m_descriptor_sets[current_frame], 0, nullptr);

    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

    m_context->EndRendering(command_buffer); // vkCmdEndRenderingKHR

    VkImageMemoryBarrier im_mem_barrier = {};
    im_mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    im_mem_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    im_mem_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    im_mem_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    im_mem_barrier.image = m_swap_chain_images[image_index];
    im_mem_barrier.subresourceRange = subresource_range;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &im_mem_barrier);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to Record Command Buffer!");
}

void Renderer::create_sync_objects()
{
    m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(m_context->m_device, &semaphore_info, nullptr,
                              &m_image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_context->m_device, &semaphore_info, nullptr,
                              &m_render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_context->m_device, &fence_info, nullptr, &m_in_flight_fences[i]) !=
                VK_SUCCESS)
        {
            throw std::runtime_error("Failed to Create Semaphores and Fences!");
        }
    }
}

void Renderer::create_vertex_buffer()
{
    VkDeviceSize buffer_size = sizeof(m_vertices[0]) * m_vertices.size();

    VkBuffer staging_buffer = {};
    VkDeviceMemory staging_buffer_memory = {};
    create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_buffer, staging_buffer_memory);

    void* data = nullptr;
    vkMapMemory(m_context->m_device, staging_buffer_memory, 0, buffer_size, 0, &data);

    memcpy(data, m_vertices.data(), (size_t)buffer_size);

    vkUnmapMemory(m_context->m_device, staging_buffer_memory);

    create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertex_buffer, m_vertex_buffer_memory);

    copy_buffer(staging_buffer, m_vertex_buffer, buffer_size);

    vkDestroyBuffer(m_context->m_device, staging_buffer, nullptr);
    vkFreeMemory(m_context->m_device, staging_buffer_memory, nullptr);
}

void Renderer::create_index_buffer()
{
    VkDeviceSize buffer_size = sizeof(m_indices[0]) * m_indices.size();

    VkBuffer staging_buffer = {};
    VkDeviceMemory staging_buffer_memory = {};
    create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_buffer, staging_buffer_memory);

    void* data;
    vkMapMemory(m_context->m_device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, m_indices.data(), (size_t)buffer_size);
    vkUnmapMemory(m_context->m_device, staging_buffer_memory);

    create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_index_buffer, m_index_buffer_memory);

    copy_buffer(staging_buffer, m_index_buffer, buffer_size);

    vkDestroyBuffer(m_context->m_device, staging_buffer, nullptr);
    vkFreeMemory(m_context->m_device, staging_buffer_memory, nullptr);
}

void Renderer::create_uniform_buffers()
{
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);

    m_uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      m_uniform_buffers[i], m_uniform_buffers_memory[i]);

        vkMapMemory(m_context->m_device, m_uniform_buffers_memory[i], 0, buffer_size, 0,
                    &m_uniform_buffers_mapped[i]);
    }
}

void Renderer::update_uniform_buffer(uint32_t current_image)
{
    static auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time)
            .count();

    UniformBufferObject ubo = {};
    ubo.model =
        glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),
                                m_swap_chain_extent.width / (float)m_swap_chain_extent.height, 0.1f,
                                10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(m_uniform_buffers_mapped[current_image], &ubo, sizeof(ubo));
}

void Renderer::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags properties, VkBuffer& buffer,
                             VkDeviceMemory& buffer_memory)
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_context->m_device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Buffer!");

    VkMemoryRequirements mem_requirements = {};
    vkGetBufferMemoryRequirements(m_context->m_device, buffer, &mem_requirements);
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = m_context->find_memory_type(
        mem_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_context->m_device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS)
        throw std::runtime_error("Failed to Allocate Vertex Buffer Memory!");

    vkBindBufferMemory(m_context->m_device, buffer, buffer_memory, 0);
}

void Renderer::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
    VkCommandBuffer command_buffer = begin_single_time_commands();

    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    end_single_time_commands(command_buffer);
}

void Renderer::create_texture_iamge()
{
    int width = -1, height = -1, channels = -1;
    stbi_uc* pixels = stbi_load("textures/photon-the-final-gathering.jpg", &width, &height,
                                &channels, STBI_rgb_alpha);
    VkDeviceSize image_size = width * height * 4;
    if (!pixels)
        throw std::runtime_error("Failed to Load Texture Image!");

    VkBuffer staging_buffer = {};
    VkDeviceMemory staging_buffer_memory = {};

    create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_buffer, staging_buffer_memory);

    void* data = nullptr;
    vkMapMemory(m_context->m_device, staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(image_size));
    vkUnmapMemory(m_context->m_device, staging_buffer_memory);

    stbi_image_free(pixels);

    create_image(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_texture_image, m_texture_image_memory);

    transition_image_layout(m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(staging_buffer, m_texture_image, static_cast<uint32_t>(width),
                         static_cast<uint32_t>(height));
    transition_image_layout(m_texture_image, VK_FORMAT_R8G8B8A8_SRGB,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_context->m_device, staging_buffer, nullptr);
    vkFreeMemory(m_context->m_device, staging_buffer_memory, nullptr);
}

void Renderer::create_texture_image_view()
{
    m_texture_image_view =
        create_image_view(m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Renderer::create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                            VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                            VkImage& image, VkDeviceMemory& image_memory)
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

    VkMemoryRequirements mem_requirements = {};
    vkGetImageMemoryRequirements(m_context->m_device, image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex =
        m_context->find_memory_type(mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_context->m_device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS)
        throw std::runtime_error("Failed to Allocate Image Memory!");

    vkBindImageMemory(m_context->m_device, image, image_memory, 0);
}

VkImageView Renderer::create_image_view(VkImage image, VkFormat format,
                                        VkImageAspectFlags aspect_flags)
{
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(m_context->m_device, &view_info, nullptr, &image_view) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return image_view;
}

void Renderer::transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout,
                                       VkImageLayout new_layout)
{
    VkCommandBuffer command_buffer = begin_single_time_commands();

    VkPipelineStageFlags source_stage = {};
    VkPipelineStageFlags destination_stage = {};

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (m_context->has_stencil_component(format))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
        throw std::invalid_argument("Unsupported Layout Transition");

    vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr,
                         1, &barrier);

    end_single_time_commands(command_buffer);
}

void Renderer::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer command_buffer = begin_single_time_commands();

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

    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &region);

    end_single_time_commands(command_buffer);
}

void Renderer::create_texture_sampler()
{
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(m_context->m_physical_device, &properties);

    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    if (vkCreateSampler(m_context->m_device, &sampler_info, nullptr, &m_texture_sampler) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Create Texture Sampler!");
}

void Renderer::create_depth_resources()
{
    VkFormat depth_format = m_context->find_depth_format();
    create_image(m_swap_chain_extent.width, m_swap_chain_extent.height, depth_format,
                 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depth_image, m_depth_image_memory);
    m_depth_image_view = create_image_view(m_depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);

    transition_image_layout(m_depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkCommandBuffer Renderer::begin_single_time_commands()
{
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = m_context->m_command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer = {};
    vkAllocateCommandBuffers(m_context->m_device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);
    return command_buffer;
}

void Renderer::end_single_time_commands(VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(m_context->m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_context->m_graphics_queue);

    vkFreeCommandBuffers(m_context->m_device, m_context->m_command_pool, 1, &command_buffer);
}

void Renderer::create_descriptor_pool()
{
    std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(m_context->m_device, &pool_info, nullptr, &m_descriptor_pool) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Create Descriptor Pool!");
}

void Renderer::create_descriptor_sets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptor_set_layout);
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = m_descriptor_pool;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    alloc_info.pSetLayouts = layouts.data();

    m_descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(m_context->m_device, &alloc_info, m_descriptor_sets.data()) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Allocate Descriptor Sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = m_uniform_buffers[i];
        buffer_info.offset = 0;
        buffer_info.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = m_texture_image_view;
        image_info.sampler = m_texture_sampler;

        std::array<VkWriteDescriptorSet, 2> descriptor_writes = {};
        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = m_descriptor_sets[i];
        descriptor_writes[0].dstBinding = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo = &buffer_info;
        descriptor_writes[0].pTexelBufferView = nullptr;
        descriptor_writes[0].pImageInfo = nullptr;

        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = m_descriptor_sets[i];
        descriptor_writes[1].dstBinding = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pBufferInfo = nullptr;
        descriptor_writes[1].pTexelBufferView = nullptr;
        descriptor_writes[1].pImageInfo = &image_info;

        vkUpdateDescriptorSets(m_context->m_device, static_cast<uint32_t>(descriptor_writes.size()),
                               descriptor_writes.data(), 0, nullptr);
    }
}

void Renderer::create_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding ubo_layout_binding = {};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding sampler_layout_binding = {};
    sampler_layout_binding.binding = 1;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {ubo_layout_binding,
                                                            sampler_layout_binding};

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = bindings.size();
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_context->m_device, &layout_info, nullptr,
                                    &m_descriptor_set_layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Descriptor Set Layout");
}

VkVertexInputBindingDescription Vertex::get_binding_description()
{
    VkVertexInputBindingDescription binding_description = {};
    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding_description;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::get_attribute_description()
{
    std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions = {};
    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(Vertex, pos);

    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(Vertex, color);

    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(Vertex, tex_coord);

    return attribute_descriptions;
}
