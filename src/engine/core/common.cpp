#include "common.hpp"

#include <stdexcept>
#include <fstream>

#include <vk_mem_alloc.h>
#include <stb_image.h>

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

Texture::Texture(const TextureDesc& desc) : Desc(desc)
{
    if (desc.Format == VK_FORMAT_UNDEFINED)
        throw std::runtime_error("Texture creation failed, invalid format!");
    if (desc.Mips < 1)
        throw std::runtime_error("Texture creation failed, invalid amount of mips!");
    if (desc.Layers < 1 || desc.Layers > 6)
        throw std::runtime_error("Texture creation failed, invalid number of layers!");
    if (desc.Type == TextureDesc::TextureType::CUBEMAP && desc.Layers != 6)
        throw std::runtime_error("Cubemaps must have exactly 6 layers!");

    const VkImageCreateFlags flags =
        desc.Type == TextureDesc::TextureType::CUBEMAP ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

    nijiEngine.m_context.create_image(desc.Width, desc.Height, desc.Mips, desc.Layers, desc.Format,
                                      VK_IMAGE_TILING_OPTIMAL, desc.Usage, desc.MemoryUsage, flags,
                                      TextureImage, TextureImageAllocation);

    nijiEngine.m_context.transition_image_layout(TextureImage, desc.Format,
                                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, desc.Mips,
                                                 desc.Layers);
    if (!desc.Data)
        throw std::runtime_error("2D texture creation failed, no pixel data provided!");

    const VkDeviceSize imageSize =
        static_cast<VkDeviceSize>(desc.Width) * desc.Height * desc.Channels;

    VkBuffer stagingBuffer = {};
    VmaAllocation stagingAlloc = {};
    nijiEngine.m_context.create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingAlloc);

    void* data = nullptr;
    vmaMapMemory(nijiEngine.m_context.m_allocator, stagingAlloc, &data);
    memcpy(data, desc.Data, static_cast<size_t>(imageSize));
    vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingAlloc);
    stbi_image_free((void*)desc.Data);

    nijiEngine.m_context.copy_buffer_to_image(stagingBuffer, TextureImage, desc.Width, desc.Height,
                                              desc.Layers, 0, 0);

    vmaDestroyBuffer(nijiEngine.m_context.m_allocator, stagingBuffer, stagingAlloc);

    nijiEngine.m_context.transition_image_layout(TextureImage, desc.Format,
                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                 desc.Mips, desc.Layers);

    TextureImageView =
        nijiEngine.m_context.create_image_view(TextureImage, desc.Format, VK_IMAGE_ASPECT_COLOR_BIT,
                                               desc.Mips, desc.Layers);

    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageInfo.imageView = TextureImageView;
    ImageInfo.sampler = VK_NULL_HANDLE;
}

Texture::Texture(const TextureDesc& desc, const std::string& path)
{
    if (desc.Type != TextureDesc::TextureType::CUBEMAP)
        throw std::runtime_error("You used the wrong constructor :/");
    if (desc.Format == VK_FORMAT_UNDEFINED)
        throw std::runtime_error("Texture creation failed, invalid format!");
    if (desc.Mips < 1)
        throw std::runtime_error("Texture creation failed, invalid amount of mips!");
    if (desc.Layers != 6)
        throw std::runtime_error("Cubemaps must have exactly 6 layers!");

    const VkImageCreateFlags flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    nijiEngine.m_context.create_image(desc.Width, desc.Height, desc.Mips, desc.Layers, desc.Format,
                                      VK_IMAGE_TILING_OPTIMAL, desc.Usage, desc.MemoryUsage, flags,
                                      TextureImage, TextureImageAllocation);

    nijiEngine.m_context.transition_image_layout(TextureImage, desc.Format,
                                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, desc.Mips,
                                                 desc.Layers);

    if (desc.Mips > 1)
    {
        // Load all faces and mips
        int w, h, n;
        std::vector<std::vector<void*>> cubemapData(desc.Mips, std::vector<void*>(6));
        for (int mip = 0; mip < desc.Mips; ++mip)
        {
            const std::string mipPath = path + "/mip" + std::to_string(mip) + "/";
            const std::string faces[6] = {"px", "nx", "py", "ny", "pz", "nz"};

            for (int face = 0; face < 6; ++face)
            {
                const std::vector<char> file = read_binary_file(mipPath + faces[face] + ".hdr");
                if (file.empty())
                    throw std::runtime_error("Failed to load face for mip " + std::to_string(mip));

                float* pixels = stbi_loadf_from_memory((const stbi_uc*)file.data(),
                                                       (int)file.size(), &w, &h, &n, 4);
                if (!pixels)
                    throw std::runtime_error("Failed to decode HDR mip " + std::to_string(mip));

                cubemapData[mip][face] = pixels;
            }
        }

        if (cubemapData.empty() || cubemapData.size() != desc.Mips)
            throw std::runtime_error("CubemapMipData must contain data for all mips!");

        for (uint32_t mip = 0; mip < desc.Mips; ++mip)
        {
            if (cubemapData[mip].size() != 6)
                throw std::runtime_error("Each mip must contain 6 face images!");

            const uint32_t mipWidth = std::max(1, desc.Width >> mip);
            const uint32_t mipHeight = std::max(1, desc.Height >> mip);
            const VkDeviceSize mipSize =
                static_cast<uint64_t>(mipWidth) * mipHeight * desc.Channels * sizeof(float);

            for (uint32_t face = 0; face < 6; ++face)
            {
                VkBuffer stagingBuffer = {};
                VmaAllocation stagingAlloc = {};
                nijiEngine.m_context.create_buffer(mipSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                   VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer,
                                                   stagingAlloc);

                void* mapped = nullptr;
                vmaMapMemory(nijiEngine.m_context.m_allocator, stagingAlloc, &mapped);
                memcpy(mapped, cubemapData[mip][face], mipSize);
                vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingAlloc);
                stbi_image_free(cubemapData[mip][face]);

                nijiEngine.m_context.copy_buffer_to_image(stagingBuffer, TextureImage, mipWidth,
                                                          mipHeight, 1, face, mip);

                vmaDestroyBuffer(nijiEngine.m_context.m_allocator, stagingBuffer, stagingAlloc);
            }
        }
    }
    else if (desc.Mips == 1) // Diffuse Cubemap
    {
        int w, h, n;
        const std::string faces[6] = {"px", "nx", "py", "ny", "pz", "nz"};

        for (uint32_t face = 0; face < 6; ++face)
        {
            const std::vector<char> file = read_binary_file(path + "/" + faces[face] + ".hdr");
            if (file.empty())
                throw std::runtime_error("Failed to load face: " + faces[face]);

            float* pixels = stbi_loadf_from_memory((const stbi_uc*)file.data(), (int)file.size(),
                                                   &w, &h, &n, 4);
            if (!pixels)
                throw std::runtime_error("Failed to decode HDR face: " + faces[face]);

            const uint32_t mipWidth = desc.Width;
            const uint32_t mipHeight = desc.Height;
            const VkDeviceSize mipSize =
                static_cast<VkDeviceSize>(mipWidth) * mipHeight * desc.Channels * sizeof(float);


            VkBuffer stagingBuffer = {};
            VmaAllocation stagingAlloc = {};
            nijiEngine.m_context.create_buffer(mipSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                               VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer,
                                               stagingAlloc);

            void* mapped = nullptr;
            vmaMapMemory(nijiEngine.m_context.m_allocator, stagingAlloc, &mapped);
            memcpy(mapped, pixels, mipSize);
            vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingAlloc);
            stbi_image_free(pixels);

            nijiEngine.m_context.copy_buffer_to_image(stagingBuffer, TextureImage, mipWidth,
                                                      mipHeight, 1, face, 0);

            vmaDestroyBuffer(nijiEngine.m_context.m_allocator, stagingBuffer, stagingAlloc);
        }
    }

    nijiEngine.m_context.transition_image_layout(TextureImage, desc.Format,
                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                 desc.Mips, desc.Layers);

    TextureImageView =
        nijiEngine.m_context.create_image_view(TextureImage, desc.Format, VK_IMAGE_ASPECT_COLOR_BIT,
                                               desc.Mips, desc.Layers);

    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageInfo.imageView = TextureImageView;
    ImageInfo.sampler = VK_NULL_HANDLE;
}

void Texture::cleanup() const
{
    vkDestroyImageView(nijiEngine.m_context.m_device, TextureImageView, nullptr);
    vmaDestroyImage(nijiEngine.m_context.m_allocator, TextureImage, TextureImageAllocation);
}

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

inline static VkFilter to_vk(SamplerDesc::Filter filterMode)
{
    switch (filterMode)
    {
    case niji::SamplerDesc::Filter::NONE:
        throw std::runtime_error("Invalid Sampler Filter Mode!");
        break;
    case niji::SamplerDesc::Filter::NEAREST:
        return VK_FILTER_NEAREST;
        break;
    case niji::SamplerDesc::Filter::LINEAR:
        return VK_FILTER_LINEAR;
        break;
    default:
        throw std::runtime_error("Invalid Sampler Filter Mode!");
        break;
    }
}

inline static VkSamplerAddressMode to_vk(SamplerDesc::AddressMode addressMode)
{
    switch (addressMode)
    {
    case niji::SamplerDesc::AddressMode::NONE:
        throw std::runtime_error("Invalid Sampler Address Mode!");
        break;
    case niji::SamplerDesc::AddressMode::REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;
    case niji::SamplerDesc::AddressMode::MIRRORED_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        break;
    case niji::SamplerDesc::AddressMode::EDGE_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        break;
    case niji::SamplerDesc::AddressMode::BORDER_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        break;
    case niji::SamplerDesc::AddressMode::MIRRORED_EDGE_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        break;
    default:
        throw std::runtime_error("Invalid Sampler Address Mode!");
        break;
    }
}

inline static VkSamplerMipmapMode to_vk(SamplerDesc::MipMapMode mipmapMode)
{
    switch (mipmapMode)
    {
    case niji::SamplerDesc::MipMapMode::NONE:
        throw std::runtime_error("Invalid Sampler Mip Map Mode!");
        break;
    case niji::SamplerDesc::MipMapMode::NEAREST:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        break;
    case niji::SamplerDesc::MipMapMode::LINEAR:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        break;
    default:
        throw std::runtime_error("Invalid Sampler Mip Map Mode!");
        break;
    }
}

Sampler::Sampler(const SamplerDesc& desc)
{
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(nijiEngine.m_context.m_physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = to_vk(desc.MagFilter);
    samplerInfo.minFilter = to_vk(desc.MinFilter);
    samplerInfo.addressModeU = to_vk(desc.AddressModeU);
    samplerInfo.addressModeV = to_vk(desc.AddressModeV);
    samplerInfo.addressModeW = to_vk(desc.AddressModeW);
    samplerInfo.anisotropyEnable = desc.EnableAnisotropy;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    // Default to these values for now
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = to_vk(desc.MipmapMode);
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(nijiEngine.m_context.m_device, &samplerInfo, nullptr, &Handle) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to Create Sampler!");
}

void Sampler::cleanup() const
{
    if (Handle != VK_NULL_HANDLE)
        vkDestroySampler(nijiEngine.m_context.m_device, Handle, nullptr);
}