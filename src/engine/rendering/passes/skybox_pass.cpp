#include "skybox_pass.hpp"

#include "rendering/renderer.hpp"

#include "../model/model.hpp"

using namespace niji;

void SkyboxPass::init(Swapchain& swapchain, Descriptor& globalDescriptor)
{
    m_name = "Skybox Pass";

    {
        SamplerDesc desc = {};
        desc.MagFilter = SamplerDesc::Filter::LINEAR;
        desc.MinFilter = SamplerDesc::Filter::LINEAR;
        desc.AddressModeU = SamplerDesc::AddressMode::REPEAT;
        desc.AddressModeV = SamplerDesc::AddressMode::REPEAT;
        desc.AddressModeW = SamplerDesc::AddressMode::REPEAT;
        desc.EnableAnisotropy = true;
        desc.MipmapMode = SamplerDesc::MipMapMode::LINEAR;

        m_sampler = Sampler(desc);
    }

    {
        DescriptorInfo descriptorInfo = {};
        descriptorInfo.IsPushDescriptor = true;

        DescriptorBinding textureBinding = {};
        textureBinding.Type = DescriptorBinding::BindType::TEXTURE;
        textureBinding.Count = 1;
        textureBinding.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
        textureBinding.Sampler = nullptr;
        descriptorInfo.Bindings.push_back(textureBinding);

        DescriptorBinding samplerBinding = {};
        samplerBinding.Type = DescriptorBinding::BindType::SAMPLER;
        samplerBinding.Count = 1;
        samplerBinding.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
        samplerBinding.Sampler = nullptr;
        descriptorInfo.Bindings.push_back(samplerBinding);

        m_passDescriptor = Descriptor(descriptorInfo);
    }

    PipelineDesc pipelineDesc = {globalDescriptor.m_setLayout, m_passDescriptor.m_setLayout};

    pipelineDesc.Name = "Skybox Pass";
    pipelineDesc.VertexShader = "shaders/spirv/skybox_pass.vert.spv";
    pipelineDesc.FragmentShader = "shaders/spirv/skybox_pass.frag.spv";

    pipelineDesc.Rasterizer.CullMode = RasterizerState::CullingMode::NONE;
    pipelineDesc.Rasterizer.PolyMode = RasterizerState::PolygonMode::FILL;
    pipelineDesc.Rasterizer.RasterizerDiscardEnable = false;
    pipelineDesc.Rasterizer.DepthClampEnable = false;
    pipelineDesc.Rasterizer.LineWidth = 1.0f;

    pipelineDesc.Topology = PipelineDesc::PrimitiveTopology::TRIANGLE_LIST;

    pipelineDesc.Viewport.Width = swapchain.m_extent.width;
    pipelineDesc.Viewport.Height = swapchain.m_extent.height;
    pipelineDesc.Viewport.MaxDepth = 1.0f;
    pipelineDesc.Viewport.MinDepth = 0.0f;
    pipelineDesc.Viewport.ScissorWidth = swapchain.m_extent.width;
    pipelineDesc.Viewport.ScissorHeight = swapchain.m_extent.height;

    pipelineDesc.ColorAttachmentFormat = swapchain.m_format;

    pipelineDesc.VertexLayout =
        DEFINE_VERTEX_LAYOUT(SkyboxVertex, VertexElement(0, VK_FORMAT_R32G32B32_SFLOAT,
                                                         offsetof(SkyboxVertex, Pos)));

    m_pipeline = Pipeline(pipelineDesc);
}

void SkyboxPass::update(Renderer& renderer)
{
}

void SkyboxPass::record(Renderer& renderer, CommandList& cmd, RenderInfo& info)
{
    Swapchain& swapchain = renderer.m_swapchain;
    const uint32_t& frameIndex = renderer.m_currentFrame;

    info.DepthAttachment.StoreOp = VK_ATTACHMENT_STORE_OP_NONE;

    cmd.begin_rendering(info, m_name);

    cmd.bind_pipeline(m_pipeline.PipelineObject);

    cmd.bind_viewport(swapchain.m_extent);
    cmd.bind_scissor(swapchain.m_extent);

    auto& mesh = renderer.m_cube;

    VkBuffer vertexBuffers[] = {mesh.m_vertexBuffer.Handle};
    VkDeviceSize offsets[] = {0};
    cmd.bind_vertex_buffer(0, 1, vertexBuffers, offsets);
    cmd.bind_index_buffer(mesh.m_indexBuffer.Handle, 0,
                          mesh.m_ushortIndices ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

    // Globals - 0
    {
        cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.PipelineLayout, 0, 1,
                                 &renderer.m_globalDescriptor.m_set[renderer.m_currentFrame]);
    }

    // Per Pass - 1
    {
        m_passDescriptor.m_info.Bindings[0].Resource = &renderer.m_envmap->m_specularCubemap;
        m_passDescriptor.m_info.Bindings[1].Resource = &m_sampler;

        std::vector<VkWriteDescriptorSet> writes = {};
        std::vector<VkDescriptorBufferInfo> bufferInfos = {};
        std::vector<VkDescriptorImageInfo> imageInfos = {};

        writes.reserve(m_passDescriptor.m_info.Bindings.size());
        bufferInfos.reserve(m_passDescriptor.m_info.Bindings.size());
        imageInfos.reserve(m_passDescriptor.m_info.Bindings.size());

        m_passDescriptor.push_descriptor_writes(writes, bufferInfos, imageInfos);

        cmd.push_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.PipelineLayout, 1,
                                static_cast<uint32_t>(writes.size()), writes.data());
    }

    cmd.draw_indexed(static_cast<uint32_t>(mesh.m_indexCount), 1, 0, 0, 0);
    cmd.end_rendering(info);
}

void SkyboxPass::cleanup()
{
    base_cleanup();
    m_sampler.cleanup();
}