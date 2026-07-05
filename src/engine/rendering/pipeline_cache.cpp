#include "pipeline_cache.hpp"

#include <filesystem>
#include <fstream>

#include "engine.hpp"

#include "renderer.hpp"
#include "resource_bank.hpp"

#include "core/context.hpp"
#include "core/common.hpp"

#include "rendergraph/nodes/compute_node.hpp"

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

    // Descriptor Layout??
    // pipeline.Descriptors = ...

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

} // namespace niji