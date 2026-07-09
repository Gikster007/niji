#include "pipeline_cache.hpp"

#include <filesystem>
#include <fstream>

#include "engine.hpp"

#include "renderer.hpp"
#include "resource_bank.hpp"

#include "core/context.hpp"
#include "core/common.hpp"

#include "rendering/utils/translate.hpp"

#include "rendering/resources/render_target.hpp"

#include "rendergraph/nodes/compute_node.hpp"
#include "rendergraph/nodes/raster_node.hpp"

namespace niji
{

namespace fs = std::filesystem;

static VkShaderModule create_shader_module(const std::string filename)
{
    // Read File
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        printf("Failed to open File!");
        throw std::runtime_error("Failed to Open File!");
    }

    size_t fileSize = (size_t)file.tellg();
    // printf("File size of %s is %zd \n", filename.c_str(), fileSize);
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    // Create Shader Module
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileSize;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    VkShaderModule shaderModule = {};
    if (vkCreateShaderModule(nijiEngine.m_context.m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Shader Module!");
    return shaderModule;
}

void PipelineCache::clear()
{
    for (const auto& [name, pipeline] : m_cache)
    {
        vkDestroyDescriptorSetLayout(nijiEngine.m_context.m_device, pipeline.Descriptors, nullptr);
        vkDestroyPipelineLayout(nijiEngine.m_context.m_device, pipeline.Layout, nullptr);
        vkDestroyPipeline(nijiEngine.m_context.m_device, pipeline.Object, nullptr);
    }
    m_cache.clear();
}

Pipeline PipelineCache::get_pipeline(const std::string_view path, const ComputeNode& node)
{
    // Check for Existing Pipeline
    const std::string key = std::string(node.m_label);
    if (m_cache.count(key) == 1u)
        return m_cache[key];

    // Create new Pipeline
    Pipeline pipeline {};
    pipeline.Name = node.m_label;

    // Create Shader Module
    std::string filename = std::string(path) + std::string(node.m_computePath) + std::string(".spv");
    VkShaderModule shaderModule = create_shader_module(filename);
    const std::string shaderName = "Compute Shader (" + std::string(node.m_computePath) + ")";
    SetObjectName(nijiEngine.m_context.m_device, VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE, shaderModule, shaderName.c_str());

    VkPushConstantRange pcRange = {};
    if (node.m_rangeSize != 0u)
    {
        pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pcRange.offset = node.m_rangeOffset;
        pcRange.size = node.m_rangeSize;
    }

    // Create Pipeline Layout
    VkPipelineLayoutCreateInfo layoutInfo {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &nijiEngine.m_renderer.m_resourceBank.m_bindlessSetLayout;
    if (node.m_rangeSize != 0)
    {
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pcRange;
    }

    if (vkCreatePipelineLayout(nijiEngine.m_context.m_device, &layoutInfo, nullptr, &pipeline.Layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline layout!");
    const std::string pipelineLayoutNname = "Compute Pipeline Layout (" + pipeline.Name + ")";
    SetObjectName(nijiEngine.m_context.m_device, VkObjectType::VK_OBJECT_TYPE_PIPELINE_LAYOUT, pipeline.Layout,
                  pipelineLayoutNname.c_str());

    // Compute Shader Stage Info
    VkPipelineShaderStageCreateInfo computeShaderStageInfo {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = shaderModule;
    computeShaderStageInfo.pName = "main";

    // Compute Pipeline Info
    VkComputePipelineCreateInfo computePipelineInfo = {};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.stage = computeShaderStageInfo;
    computePipelineInfo.layout = pipeline.Layout;
    computePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    computePipelineInfo.basePipelineIndex = -1;

    // Create Compute Pipeline
    if (vkCreateComputePipelines(nijiEngine.m_context.m_device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr,
                                 &pipeline.Object) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline!");
    const std::string pipelineName = "Compute Pipeline (" + pipeline.Name + ")";
    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_PIPELINE, pipeline.Object, pipelineName.c_str());

    // No Longer Needed
    vkDestroyShaderModule(nijiEngine.m_context.m_device, shaderModule, nullptr);

    // Insert New Pipeline into Cache and Return
    m_cache[key] = pipeline;
    return pipeline;
}

Pipeline PipelineCache::get_pipeline(const std::string_view path, const RasterNode& node)
{
    // Check for Existing Pipeline
    const std::string key = std::string(node.m_label);
    if (m_cache.count(key) == 1u)
        return m_cache[key];

    // Create new Pipeline
    Pipeline pipeline {};
    pipeline.Name = node.m_label;

    // Create Shader Modules
    std::string vertexFilename = std::string(path) + std::string(node.m_shaderPath) + std::string(".vert.spv");
    VkShaderModule vertexModule = create_shader_module(vertexFilename);
    const std::string vertexName = "Vertex Shader (" + std::string(node.m_shaderPath) + ".vert)";
    SetObjectName(nijiEngine.m_context.m_device, VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE, vertexModule, vertexName.c_str());

    std::string fragmentFilename = std::string(path) + std::string(node.m_shaderPath) + std::string(".frag.spv");
    VkShaderModule fragmentModule = create_shader_module(fragmentFilename);
    const std::string fragmentName = "Fragment Shader (" + std::string(node.m_shaderPath) + ".frag)";
    SetObjectName(nijiEngine.m_context.m_device, VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE, fragmentModule, fragmentName.c_str());

    VkPushConstantRange pcRange = {};
    if (node.m_rangeSize != 0u)
    {
        pcRange.stageFlags = translate::to_vk_shader_stages(node.m_pcStages);
        pcRange.offset = node.m_rangeOffset;
        pcRange.size = node.m_rangeSize;
    }

    // Create Pipeline Layout
    VkPipelineLayoutCreateInfo layoutInfo {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &nijiEngine.m_renderer.m_resourceBank.m_bindlessSetLayout;
    if (node.m_rangeSize != 0)
    {
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pcRange;
    }

    if (vkCreatePipelineLayout(nijiEngine.m_context.m_device, &layoutInfo, nullptr, &pipeline.Layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create raster pipeline layout!");
    const std::string pipelineLayoutNname = "Graphics Pipeline Layout (" + pipeline.Name + ")";
    SetObjectName(nijiEngine.m_context.m_device, VkObjectType::VK_OBJECT_TYPE_PIPELINE_LAYOUT, pipeline.Layout, pipelineLayoutNname.c_str());

    // Vertex + Fragment Shader Stages Info
    VkPipelineShaderStageCreateInfo stages[2] {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertexModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragmentModule;
    stages[1].pName = "main";

    // Pipeline Vertex Input State
    VkPipelineVertexInputStateCreateInfo vertexInput {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = 0u;
    vertexInput.pVertexBindingDescriptions = nullptr;
    vertexInput.vertexAttributeDescriptionCount = 0u;
    vertexInput.pVertexAttributeDescriptions = nullptr;

    // Pipeline Input Assembly State
    VkPipelineInputAssemblyStateCreateInfo assemblyInput {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assemblyInput.topology = translate::primitive_topology(node.m_topology);

    // Pipeline Tessellation State
    VkPipelineTessellationStateCreateInfo tessellationState {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};

    const VkViewport viewport {};
    const VkRect2D scissor {};

    // Pipeline Viewport State
    VkPipelineViewportStateCreateInfo viewportState {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1u;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1u;
    viewportState.pScissors = &scissor;

    // Pipeline Rasterizer State
    VkPipelineRasterizationStateCreateInfo rasterState {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterState.cullMode = VK_CULL_MODE_NONE;
    rasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterState.lineWidth = 1.0f;

    // Pipeline Multisample State
    VkPipelineMultisampleStateCreateInfo multisampleState {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Find All Color/Blend Attachments
    std::vector<VkFormat> colorAttachments {};
    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments {};
    for (const Dependency& dep : node.m_dependencies)
    {
        /* Find attachment dependencies */
        if (dep.Usage != DependencyUsage::ColorAttachment)
            continue;

        VkFormat format {}; // Get the image format for render target or texture
        if (dep.Resource.Type == ResourceType::RenderTarget)
        {
            RenderTargetHandle rtHandle = reinterpret_cast<const RenderTargetHandle&>(dep.Resource);
            format = nijiEngine.m_renderer.m_resourceBank.m_renderTargets.get(rtHandle).SurfaceFormat;
        }
        else
        {
            TextureHandle texHandle = reinterpret_cast<const TextureHandle&>(dep.Resource);
            const Texture& tex = nijiEngine.m_renderer.m_resourceBank.m_textures.get(texHandle);
            if (has_flag(tex.Desc.Usage, TextureUsage::ColorAttachment) == false)
                continue;
            format = translate::texture_format(tex.Desc.Format);
        }

        colorAttachments.emplace_back(format);
        VkPipelineColorBlendAttachmentState blend_state {};
        blend_state.blendEnable = node.m_alphaBlend ? VK_TRUE : VK_FALSE;
        blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_state.colorBlendOp = VK_BLEND_OP_ADD;
        blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
        blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | (node.m_alphaBlend ? 0u : VK_COLOR_COMPONENT_A_BIT);
        blendAttachments.emplace_back(blend_state);
    }

    // Dynamic Rendering Info
    VkPipelineRenderingCreateInfoKHR dynamicRendering {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
    dynamicRendering.colorAttachmentCount = (uint32_t)colorAttachments.size();
    dynamicRendering.pColorAttachmentFormats = colorAttachments.data();

    // Pipeline DepthStencil State
    VkPipelineDepthStencilStateCreateInfo depthStencilState {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencilState.depthTestEnable = VK_FALSE;
    depthStencilState.depthWriteEnable = VK_FALSE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

    // Check for a Depth Attachment
    if (node.m_depthStencilImage.is_valid())
    {
        const Texture& depthTex = nijiEngine.m_renderer.m_resourceBank.m_textures.get(node.m_depthStencilImage);

        depthStencilState.depthTestEnable = node.m_depthTest;
        depthStencilState.depthWriteEnable = node.m_depthWrite;
        dynamicRendering.depthAttachmentFormat = translate::texture_format(depthTex.Desc.Format);

        // Check for a Stencil Attachment
        if (translate::is_stencil_format(depthTex.Desc.Format))
        {
            depthStencilState.stencilTestEnable = node.m_stencilState.Test;
            dynamicRendering.stencilAttachmentFormat = translate::texture_format(depthTex.Desc.Format);

            depthStencilState.front = translate::stencil_op_state(node.m_stencilState);
            depthStencilState.back = depthStencilState.front; // support only front face stencil op
        }
    }

    // Pipeline Color Blend State
    VkPipelineColorBlendStateCreateInfo colorBlendState {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlendState.attachmentCount = (uint32_t)blendAttachments.size();
    colorBlendState.pAttachments = blendAttachments.data();

    // Pipeline Dynamic State
    const VkDynamicState dynamicStates[] {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
    dynamic_state.pDynamicStates = dynamicStates;

    // Graphics Pipeline Info
    VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {};
    graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineInfo.pNext = &dynamicRendering;
    graphicsPipelineInfo.stageCount = 2u;
    graphicsPipelineInfo.pStages = stages;
    graphicsPipelineInfo.pVertexInputState = &vertexInput;
    graphicsPipelineInfo.pInputAssemblyState = &assemblyInput;
    graphicsPipelineInfo.pTessellationState = &tessellationState;
    graphicsPipelineInfo.pViewportState = &viewportState;
    graphicsPipelineInfo.pRasterizationState = &rasterState;
    graphicsPipelineInfo.pMultisampleState = &multisampleState;
    graphicsPipelineInfo.pDepthStencilState = &depthStencilState;
    graphicsPipelineInfo.pColorBlendState = &colorBlendState;
    graphicsPipelineInfo.pDynamicState = &dynamic_state;
    graphicsPipelineInfo.layout = pipeline.Layout;
    graphicsPipelineInfo.renderPass = nullptr;
    graphicsPipelineInfo.subpass = 0u;
    graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineInfo.basePipelineIndex = -1;

    // Create Graphics Pipeline
    if (vkCreateGraphicsPipelines(nijiEngine.m_context.m_device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &pipeline.Object) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline!");
    const std::string pipelineName = "Graphics Pipeline (" + pipeline.Name + ")";
    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_PIPELINE, pipeline.Object, pipelineName.c_str());

    // No Longer Needed
    vkDestroyShaderModule(nijiEngine.m_context.m_device, vertexModule, nullptr);
    vkDestroyShaderModule(nijiEngine.m_context.m_device, fragmentModule, nullptr);

    // Insert New Pipeline into Cache and Return
    m_cache[key] = pipeline;
    return pipeline;
}

} // namespace niji