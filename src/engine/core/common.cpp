#include "common.hpp"

#include <stdexcept>
#include <fstream>

#include <vk_mem_alloc.h>

#include "engine.hpp"

using namespace niji;

Buffer::Buffer(BufferDesc& desc, void* data)
{
    Desc = desc;
    Data = data;
    VkBuffer stagingBuffer = {};
    VmaAllocation stagingBufferAllocation = {};

    VkBufferUsageFlags usageFlags = {};
    VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (desc.Usage != BufferDesc::BufferUsage::Uniform)
    {
        usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        nijiEngine.m_context.create_buffer(desc.Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer,
                                           stagingBufferAllocation);

        void* dataStaged = nullptr;
        vmaMapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation, &dataStaged);
        memcpy(dataStaged, Data, (size_t)desc.Size);
        vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation);
    }

    switch (desc.Usage)
    {
    case BufferDesc::BufferUsage::Invalid:
        throw std::runtime_error("Failed to Create Buffer. Invalid Usage Set!");
        break;
    case BufferDesc::BufferUsage::Vertex:
        usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        break;
    case BufferDesc::BufferUsage::Index:
        usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;
    case BufferDesc::BufferUsage::Uniform:
        usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        break;
    default:
        break;
    }

    nijiEngine.m_context.create_buffer(desc.Size, usageFlags, memUsage, Handle, BufferAllocation,
                                       desc.IsPersistent);
    vmaSetAllocationName(nijiEngine.m_context.m_allocator, BufferAllocation, desc.Name);

    if (desc.Usage != BufferDesc::BufferUsage::Uniform)
    {
        nijiEngine.m_context.copy_buffer(stagingBuffer, Handle, desc.Size);
        vmaDestroyBuffer(nijiEngine.m_context.m_allocator, stagingBuffer, stagingBufferAllocation);
    }
    else if (desc.Usage == BufferDesc::BufferUsage::Uniform)
    {
        vmaMapMemory(nijiEngine.m_context.m_allocator, BufferAllocation, &Data);
        Mapped = true;
    }

    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_BUFFER, Handle, desc.Name);
}

Buffer::~Buffer()
{
    cleanup();
}

Buffer::Buffer(Buffer&& other) noexcept
{
    *this = std::move(other);
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other)
    {
        cleanup();

        Handle = other.Handle;
        BufferAllocation = other.BufferAllocation;
        Desc = std::move(other.Desc);
        Data = other.Data;
        Mapped = other.Mapped;

        other.Handle = VK_NULL_HANDLE;
        other.BufferAllocation = nullptr;
        other.Data = nullptr;
        other.Mapped = false;
    }
    return *this;
}

void Buffer::cleanup()
{
    if (BufferAllocation != nullptr)
    {
        if (Mapped)
        {
            vmaUnmapMemory(nijiEngine.m_context.m_allocator, BufferAllocation);
            Mapped = false;
        }

        if (Handle != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(nijiEngine.m_context.m_allocator, Handle, BufferAllocation);
        }

        Handle = VK_NULL_HANDLE;
        BufferAllocation = nullptr;
        Data = nullptr;
    }
}

static std::vector<char> read_file(const std::string& filename)
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

static VkShaderModule create_shader_module(VkDevice& device, const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = {};
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Shader Module!");
    return shaderModule;
}

static VkPrimitiveTopology get_primitive_topology(PipelineDesc::PrimitiveTopology& primTopology)
{
    switch (primTopology)
    {
    case PipelineDesc::PrimitiveTopology::POINT:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    case PipelineDesc::PrimitiveTopology::LINE_LIST:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case PipelineDesc::PrimitiveTopology::LINE_STRIP:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        break;
    case PipelineDesc::PrimitiveTopology::TRIANGLE_LIST:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case PipelineDesc::PrimitiveTopology::TRIANGLE_STRIP:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        break;
    default:
        break;
    }

    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

static VkPolygonMode get_polygon_mode(RasterizerState::PolygonMode& polyMode)
{
    switch (polyMode)
    {
    case RasterizerState::PolygonMode::FILL:
        return VK_POLYGON_MODE_FILL;
        break;
    case RasterizerState::PolygonMode::LINE:
        return VK_POLYGON_MODE_LINE;
        break;
    case RasterizerState::PolygonMode::POINT:
        return VK_POLYGON_MODE_POINT;
        break;
    default:
        break;
    }

    return VK_POLYGON_MODE_FILL;
}

static VkCullModeFlags get_cull_mode(RasterizerState::CullingMode& cullMode)
{
    switch (cullMode)
    {
    case RasterizerState::CullingMode::NONE:
        return VK_CULL_MODE_NONE;
        break;
    case RasterizerState::CullingMode::FRONT:
        return VK_CULL_MODE_FRONT_BIT;
        break;
    case RasterizerState::CullingMode::BACK:
        return VK_CULL_MODE_BACK_BIT;
        break;
    case RasterizerState::CullingMode::FRONT_AND_BACK:
        return VK_CULL_MODE_FRONT_AND_BACK;
        break;
    default:
        break;
    }

    return VK_CULL_MODE_NONE;
}

Pipeline::Pipeline(PipelineDesc& desc)
{
    Name = desc.Name;

    auto vertShaderCode = read_file(desc.VertexShader);
    auto fragShaderCode = read_file(desc.FragmentShader);

    VkShaderModule vertShaderModule =
        create_shader_module(nijiEngine.m_context.m_device, vertShaderCode);
    VkShaderModule fragShaderModule =
        create_shader_module(nijiEngine.m_context.m_device, fragShaderCode);

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

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &desc.VertexLayout.Binding;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(desc.VertexLayout.Attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = desc.VertexLayout.Attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = get_primitive_topology(desc.Topology);
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = desc.Viewport.Width;
    viewport.height = desc.Viewport.Height;
    viewport.minDepth = desc.Viewport.MinDepth;
    viewport.maxDepth = desc.Viewport.MaxDepth;
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {desc.Viewport.ScissorWidth, desc.Viewport.ScissorHeight};

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = desc.Rasterizer.DepthClampEnable;
    rasterizer.rasterizerDiscardEnable = desc.Rasterizer.RasterizerDiscardEnable;
    rasterizer.polygonMode = get_polygon_mode(desc.Rasterizer.PolyMode);
    rasterizer.lineWidth = desc.Rasterizer.LineWidth;
    rasterizer.cullMode = get_cull_mode(desc.Rasterizer.CullMode);
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

    std::vector<VkDescriptorSetLayout> setLayouts = {desc.GlobalDescriptorSetLayout,
                                                     desc.PassDescriptorSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = setLayouts.size();
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(nijiEngine.m_context.m_device, &pipelineLayoutInfo, nullptr,
                               &PipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Pipeline Layout!");

    VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &desc.ColorAttachmentFormat;
    pipelineRenderingInfo.depthAttachmentFormat = nijiEngine.m_context.find_depth_format();

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
    pipelineInfo.layout = PipelineLayout;
    pipelineInfo.renderPass = nullptr /*m_render_pass*/;
    pipelineInfo.pNext = &pipelineRenderingInfo;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(nijiEngine.m_context.m_device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &PipelineObject) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Graphics Pipeline!");

    vkDestroyShaderModule(nijiEngine.m_context.m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(nijiEngine.m_context.m_device, vertShaderModule, nullptr);
}

void Pipeline::cleanup()
{
    if (PipelineObject)
        vkDestroyPipeline(nijiEngine.m_context.m_device, PipelineObject, nullptr);
    if (PipelineLayout)
        vkDestroyPipelineLayout(nijiEngine.m_context.m_device, PipelineLayout, nullptr);
    Name = nullptr;
}

void NijiTexture::cleanup() const
{
    vkDestroyImageView(nijiEngine.m_context.m_device, TextureImageView, nullptr);
    vmaDestroyImage(nijiEngine.m_context.m_allocator, TextureImage, TextureImageAllocation);
}