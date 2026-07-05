#include "common.hpp"

#include <filesystem>
#include <stdexcept>
#include <fstream>

#include <imgui_impl_vulkan.h>
#include <vk_mem_alloc.h>
#include <stb_image.h>

#include "engine.hpp"
#include "context.hpp"


//inline static VkPrimitiveTopology to_vk(GraphicsPipelineDesc::PrimitiveTopology& primTopology)
//{
//    switch (primTopology)
//    {
//    case GraphicsPipelineDesc::PrimitiveTopology::POINT:
//        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
//        break;
//    case GraphicsPipelineDesc::PrimitiveTopology::LINE_LIST:
//        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
//        break;
//    case GraphicsPipelineDesc::PrimitiveTopology::LINE_STRIP:
//        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
//        break;
//    case GraphicsPipelineDesc::PrimitiveTopology::TRIANGLE_LIST:
//        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//        break;
//    case GraphicsPipelineDesc::PrimitiveTopology::TRIANGLE_STRIP:
//        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
//        break;
//    default:
//        break;
//    }
//
//    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//}

//inline static VkPolygonMode to_vk(RasterizerState::PolygonMode& polyMode)
//{
//    switch (polyMode)
//    {
//    case RasterizerState::PolygonMode::FILL:
//        return VK_POLYGON_MODE_FILL;
//        break;
//    case RasterizerState::PolygonMode::LINE:
//        return VK_POLYGON_MODE_LINE;
//        break;
//    case RasterizerState::PolygonMode::POINT:
//        return VK_POLYGON_MODE_POINT;
//        break;
//    default:
//        break;
//    }
//
//    return VK_POLYGON_MODE_FILL;
//}
//
//inline static VkCullModeFlags to_vk(RasterizerState::CullingMode& cullMode)
//{
//    switch (cullMode)
//    {
//    case RasterizerState::CullingMode::NONE:
//        return VK_CULL_MODE_NONE;
//        break;
//    case RasterizerState::CullingMode::FRONT:
//        return VK_CULL_MODE_FRONT_BIT;
//        break;
//    case RasterizerState::CullingMode::BACK:
//        return VK_CULL_MODE_BACK_BIT;
//        break;
//    case RasterizerState::CullingMode::FRONT_AND_BACK:
//        return VK_CULL_MODE_FRONT_AND_BACK;
//        break;
//    default:
//        break;
//    }
//
//    return VK_CULL_MODE_NONE;
//}
//
//inline static VkCompareOp to_vk(GraphicsPipelineDesc::DepthCompareOp compareOp)
//{
//    switch (compareOp)
//    {
//    case niji::GraphicsPipelineDesc::DepthCompareOp::NEVER:
//        return VK_COMPARE_OP_NEVER;
//        break;
//    case niji::GraphicsPipelineDesc::DepthCompareOp::LESS:
//        return VK_COMPARE_OP_LESS;
//        break;
//    case niji::GraphicsPipelineDesc::DepthCompareOp::EQUAL:
//        return VK_COMPARE_OP_EQUAL;
//        break;
//    case niji::GraphicsPipelineDesc::DepthCompareOp::LESS_OR_EQUAL:
//        return VK_COMPARE_OP_LESS_OR_EQUAL;
//        break;
//    case niji::GraphicsPipelineDesc::DepthCompareOp::GREATER:
//        return VK_COMPARE_OP_GREATER;
//        break;
//    case niji::GraphicsPipelineDesc::DepthCompareOp::NOT_EQUAL:
//        return VK_COMPARE_OP_NOT_EQUAL;
//        break;
//    case niji::GraphicsPipelineDesc::DepthCompareOp::GREATER_OR_EQUAL:
//        return VK_COMPARE_OP_GREATER_OR_EQUAL;
//        break;
//    case niji::GraphicsPipelineDesc::DepthCompareOp::ALWAYS:
//        return VK_COMPARE_OP_ALWAYS;
//        break;
//    default:
//        return VK_COMPARE_OP_LESS;
//        break;
//    }
//}
//
//Pipeline::Pipeline(GraphicsPipelineDesc desc)
//{
//    Name = desc.Name;
//    GraphicsDesc = desc;
//
//    auto vertShaderCode = read_file(desc.VertexShader);
//    auto fragShaderCode = read_file(desc.FragmentShader);
//
//    VkShaderModule vertShaderModule =
//        create_shader_module(nijiEngine.m_context.m_device, vertShaderCode);
//    VkShaderModule fragShaderModule =
//        create_shader_module(nijiEngine.m_context.m_device, fragShaderCode);
//
//    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
//    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
//    vertShaderStageInfo.module = vertShaderModule;
//    vertShaderStageInfo.pName = "main";
//
//    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
//    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//    fragShaderStageInfo.module = fragShaderModule;
//    fragShaderStageInfo.pName = "main";
//
//    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
//
//    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
//                                                 VK_DYNAMIC_STATE_SCISSOR};
//
//    VkPipelineDynamicStateCreateInfo dynamicState = {};
//    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
//    dynamicState.pDynamicStates = dynamicStates.data();
//
//    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
//    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//    vertexInputInfo.vertexBindingDescriptionCount = 1;
//    vertexInputInfo.pVertexBindingDescriptions = &desc.VertexLayout.Binding;
//    vertexInputInfo.vertexAttributeDescriptionCount =
//        static_cast<uint32_t>(desc.VertexLayout.Attributes.size());
//    vertexInputInfo.pVertexAttributeDescriptions = desc.VertexLayout.Attributes.data();
//
//    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
//    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//    inputAssembly.topology = to_vk(desc.Topology);
//    inputAssembly.primitiveRestartEnable = VK_FALSE;
//
//    VkViewport viewport = {};
//    viewport.x = 0.0f;
//    viewport.y = 0.0f;
//    viewport.width = desc.Viewport.Width;
//    viewport.height = desc.Viewport.Height;
//    viewport.minDepth = desc.Viewport.MinDepth;
//    viewport.maxDepth = desc.Viewport.MaxDepth;
//    VkRect2D scissor = {};
//    scissor.offset = {0, 0};
//    scissor.extent = {desc.Viewport.ScissorWidth, desc.Viewport.ScissorHeight};
//
//    VkPipelineViewportStateCreateInfo viewportState = {};
//    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//    viewportState.viewportCount = 1;
//    viewportState.pViewports = &viewport;
//    viewportState.scissorCount = 1;
//    viewportState.pScissors = &scissor;
//
//    VkPipelineRasterizationStateCreateInfo rasterizer = {};
//    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//    rasterizer.depthClampEnable = desc.Rasterizer.DepthClampEnable;
//    rasterizer.rasterizerDiscardEnable = desc.Rasterizer.RasterizerDiscardEnable;
//    rasterizer.polygonMode = to_vk(desc.Rasterizer.PolyMode);
//    rasterizer.lineWidth = desc.Rasterizer.LineWidth;
//    rasterizer.cullMode = to_vk(desc.Rasterizer.CullMode);
//    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//    rasterizer.depthBiasEnable = VK_FALSE;
//    rasterizer.depthBiasConstantFactor = 0.0f;
//    rasterizer.depthBiasClamp = 0.0f;
//    rasterizer.depthBiasSlopeFactor = 0.0f;
//
//    VkPipelineMultisampleStateCreateInfo multisampling = {};
//    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//    multisampling.sampleShadingEnable = VK_FALSE;
//    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//    multisampling.minSampleShading = 1.0f;
//    multisampling.pSampleMask = nullptr;
//    multisampling.alphaToCoverageEnable = VK_FALSE;
//    multisampling.alphaToOneEnable = VK_FALSE;
//
//    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
//    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
//                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//    colorBlendAttachment.blendEnable = VK_FALSE;
//    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
//    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
//    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
//    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
//
//    VkPipelineColorBlendStateCreateInfo colorBlending = {};
//    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//    colorBlending.logicOpEnable = VK_FALSE;
//    colorBlending.logicOp = VK_LOGIC_OP_COPY;
//    colorBlending.attachmentCount = 1;
//    colorBlending.pAttachments = &colorBlendAttachment;
//    colorBlending.blendConstants[0] = 0.0f;
//    colorBlending.blendConstants[1] = 0.0f;
//    colorBlending.blendConstants[2] = 0.0f;
//    colorBlending.blendConstants[3] = 0.0f;
//
//    std::vector<VkDescriptorSetLayout> setLayouts = {desc.GlobalDescriptorSetLayout,
//                                                     desc.PassDescriptorSetLayout};
//
//    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
//    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    pipelineLayoutInfo.setLayoutCount = setLayouts.size();
//    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
//    pipelineLayoutInfo.pushConstantRangeCount = 0;
//    pipelineLayoutInfo.pPushConstantRanges = nullptr;
//
//    if (vkCreatePipelineLayout(nijiEngine.m_context.m_device, &pipelineLayoutInfo, nullptr,
//                               &PipelineLayout) != VK_SUCCESS)
//        throw std::runtime_error("Failed to Create Graphics Pipeline Layout!");
//
//    VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {};
//    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
//    pipelineRenderingInfo.colorAttachmentCount = desc.ColorAttachmentCount;
//    pipelineRenderingInfo.pColorAttachmentFormats = &desc.ColorAttachmentFormat;
//    pipelineRenderingInfo.depthAttachmentFormat = nijiEngine.m_context.find_depth_format();
//
//    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
//    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//    depthStencil.depthTestEnable = desc.DepthTestEnable;
//    depthStencil.depthWriteEnable = desc.DepthWriteEnable;
//    depthStencil.depthCompareOp = to_vk(desc.DepthCompareOperation);
//    depthStencil.depthBoundsTestEnable = VK_FALSE;
//    depthStencil.minDepthBounds = 0.0f;
//    depthStencil.maxDepthBounds = 1.0f;
//    depthStencil.stencilTestEnable = VK_FALSE;
//    depthStencil.front = {};
//    depthStencil.back = {};
//
//    VkGraphicsPipelineCreateInfo pipelineInfo = {};
//    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//    pipelineInfo.stageCount = 2;
//    pipelineInfo.pStages = shaderStages;
//    pipelineInfo.pVertexInputState = &vertexInputInfo;
//    pipelineInfo.pInputAssemblyState = &inputAssembly;
//    pipelineInfo.pViewportState = &viewportState;
//    pipelineInfo.pRasterizationState = &rasterizer;
//    pipelineInfo.pMultisampleState = &multisampling;
//    pipelineInfo.pDepthStencilState = &depthStencil;
//    pipelineInfo.pColorBlendState = &colorBlending;
//    pipelineInfo.pDynamicState = &dynamicState;
//    pipelineInfo.layout = PipelineLayout;
//    pipelineInfo.renderPass = nullptr /*m_render_pass*/;
//    pipelineInfo.pNext = &pipelineRenderingInfo;
//    pipelineInfo.subpass = 0;
//    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
//    pipelineInfo.basePipelineIndex = -1;
//
//    if (vkCreateGraphicsPipelines(nijiEngine.m_context.m_device, VK_NULL_HANDLE, 1, &pipelineInfo,
//                                  nullptr, &PipelineObject) != VK_SUCCESS)
//        throw std::runtime_error("Failed to Create Graphics Pipeline!");
//
//    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_PIPELINE, PipelineObject, Name);
//
//    vkDestroyShaderModule(nijiEngine.m_context.m_device, fragShaderModule, nullptr);
//    vkDestroyShaderModule(nijiEngine.m_context.m_device, vertShaderModule, nullptr);
//}
//
//Pipeline::Pipeline(ComputePipelineDesc desc)
//{
//    Name = desc.Name;
//    ComputeDesc = desc;
//    IsGraphicsPipeline = false;
//
//    auto computeShaderCode = read_file(desc.ComputeShader);
//
//    VkShaderModule computeShaderModule =
//        create_shader_module(nijiEngine.m_context.m_device, computeShaderCode);
//
//    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
//    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
//    computeShaderStageInfo.module = computeShaderModule;
//    computeShaderStageInfo.pName = "main";
//
//    std::vector<VkDescriptorSetLayout> setLayouts = {desc.GlobalDescriptorSetLayout,
//                                                     desc.PassDescriptorSetLayout};
//
//    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
//    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
//    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
//    pipelineLayoutInfo.pushConstantRangeCount = 0;
//    pipelineLayoutInfo.pPushConstantRanges = nullptr;
//
//    if (vkCreatePipelineLayout(nijiEngine.m_context.m_device, &pipelineLayoutInfo, nullptr,
//                               &PipelineLayout) != VK_SUCCESS)
//        throw std::runtime_error("Failed to create compute pipeline layout!");
//
//    VkComputePipelineCreateInfo computePipelineInfo = {};
//    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
//    computePipelineInfo.stage = computeShaderStageInfo;
//    computePipelineInfo.layout = PipelineLayout;
//    computePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
//    computePipelineInfo.basePipelineIndex = -1;
//
//    if (vkCreateComputePipelines(nijiEngine.m_context.m_device, VK_NULL_HANDLE, 1,
//                                 &computePipelineInfo, nullptr, &PipelineObject) != VK_SUCCESS)
//        throw std::runtime_error("Failed to create compute pipeline!");
//
//    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_PIPELINE, PipelineObject, Name);
//
//    vkDestroyShaderModule(nijiEngine.m_context.m_device, computeShaderModule, nullptr);
//}
//
//void Pipeline::cleanup()
//{
//    if (PipelineObject)
//        vkDestroyPipeline(nijiEngine.m_context.m_device, PipelineObject, nullptr);
//    if (PipelineLayout)
//        vkDestroyPipelineLayout(nijiEngine.m_context.m_device, PipelineLayout, nullptr);
//    Name = nullptr;
//}

uint32_t niji::GetBytesPerTexel(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 16;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return 12;
    case VK_FORMAT_R32G32_SFLOAT:
        return 8;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return 4;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return 4;
    default:
        throw std::runtime_error("Unknown format!");
    }
}

std::vector<char> niji::read_binary_file(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        printf("File %s  was not found!", path.c_str());
        return std::vector<char>();
    }
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size))
        return buffer;
    assert(false);
    return std::vector<char>();
}

//TransitionInfo niji::usage_to_barrier(TransitionType usage, VkFormat format)
//{
//    switch (usage)
//    {
//    case TransitionType::Undefined:
//        return {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED};
//    case TransitionType::ColorAttachment:
//        return {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
//                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
//    case TransitionType::DepthStencilAttachmentWrite:
//        return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
//                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
//                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
//    case TransitionType::DepthStencilAttachmentLate:
//        return {VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
//                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
//                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
//    case TransitionType::ShaderRead:
//        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
//            return {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
//                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
//        else
//            return {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
//                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
//    case TransitionType::Present:
//        return {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
//    }
//    // Fallback:
//    return {};
//}
