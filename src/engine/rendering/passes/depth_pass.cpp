#include "depth_pass.hpp"

#include "core/components/render-components.hpp"
#include "core/components/transform.hpp"

#include "rendering/renderer.hpp"
#include "engine.hpp"

using namespace niji;

void DepthPass::init(Swapchain& swapchain, Descriptor& globalDescriptor)
{
    m_name = "Depth Pass";

    {
        DescriptorInfo descriptorInfo = {};
        descriptorInfo.IsPushDescriptor = true;
        descriptorInfo.Name = "Depth Pass Descriptor";

        DescriptorBinding materialBinding = {};
        materialBinding.Type = DescriptorBinding::BindType::UBO;
        materialBinding.Count = 1;
        materialBinding.Stage = DescriptorBinding::BindStage::VERTEX_SHADER;
        materialBinding.Sampler = nullptr;
        descriptorInfo.Bindings.push_back(materialBinding);

        m_passDescriptor = Descriptor(descriptorInfo);
    }

    GraphicsPipelineDesc pipelineDesc = {globalDescriptor.m_setLayout,
                                         m_passDescriptor.m_setLayout};

    add_shader("shaders/depth_pass.slang", ShaderType::FRAG_AND_VERT);
    pipelineDesc.Name = "Depth Pass";
    pipelineDesc.VertexShader = m_vertFrag.Spirv[0];
    pipelineDesc.FragmentShader = m_vertFrag.Spirv[1];

    pipelineDesc.Rasterizer.CullMode = RasterizerState::CullingMode::NONE;
    pipelineDesc.Rasterizer.PolyMode = RasterizerState::PolygonMode::FILL;
    pipelineDesc.Rasterizer.RasterizerDiscardEnable = false;
    pipelineDesc.Rasterizer.DepthClampEnable = false;
    pipelineDesc.Rasterizer.LineWidth = 1.0f;

    pipelineDesc.Topology = GraphicsPipelineDesc::PrimitiveTopology::TRIANGLE_LIST;

    pipelineDesc.Viewport.Width = swapchain.m_extent.width;
    pipelineDesc.Viewport.Height = swapchain.m_extent.height;
    pipelineDesc.Viewport.MaxDepth = 1.0f;
    pipelineDesc.Viewport.MinDepth = 0.0f;
    pipelineDesc.Viewport.ScissorWidth = swapchain.m_extent.width;
    pipelineDesc.Viewport.ScissorHeight = swapchain.m_extent.height;

    pipelineDesc.DepthTestEnable = true;
    pipelineDesc.DepthWriteEnable = true;
    pipelineDesc.DepthCompareOperation = GraphicsPipelineDesc::DepthCompareOp::LESS_OR_EQUAL;

    pipelineDesc.ColorAttachmentFormat = swapchain.m_format;
    pipelineDesc.ColorAttachmentCount = 0;

    pipelineDesc.VertexLayout =
        DEFINE_VERTEX_LAYOUT(Vertex,
                             VertexElement(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Pos)),
                             VertexElement(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Color)),
                             VertexElement(2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal)),
                             VertexElement(3, VK_FORMAT_R32G32B32A32_SFLOAT,
                                           offsetof(Vertex, Tangent)),
                             VertexElement(4, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, TexCoord)));

    m_pipelines.emplace(pipelineDesc.Name, Pipeline(pipelineDesc));
}

void DepthPass::update_impl(Renderer& renderer, CommandList& cmd)
{
    const uint32_t& frameIndex = renderer.m_currentFrame;
    {
        ModelData ubo = {};

        auto view = nijiEngine.ecs.m_registry.view<Transform, MeshComponent>();
        for (auto&& [entity, trans, mesh] : view.each())
        {
            auto& model = mesh.Model;
            auto& modelMesh = model->m_meshes[mesh.MeshID];
            auto& material = model->m_materials[mesh.MaterialID];

            ubo.Model = trans.World();
            ubo.InvModel = glm::transpose(glm::inverse(ubo.Model));

            ubo.MaterialInfo = material.m_materialInfo;

            vkCmdUpdateBuffer(cmd.m_commandBuffer, material.m_data[frameIndex].Handle, 0,
                              sizeof(ModelData), &ubo);
        }
    }
}

void DepthPass::record(Renderer& renderer, CommandList& cmd, RenderInfo& info)
{
    Swapchain& swapchain = renderer.m_swapchain;
    const uint32_t& frameIndex = renderer.m_currentFrame;
    const Pipeline& pipeline = m_pipelines.at("Depth Pass");

    info.ColorAttachment->StoreOp = VK_ATTACHMENT_STORE_OP_NONE;
    info.ColorAttachment->LoadOp = VK_ATTACHMENT_LOAD_OP_NONE;

    info.DepthAttachment->StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    info.DepthAttachment->LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    cmd.begin_rendering(info, m_name);

    cmd.bind_pipeline(pipeline.PipelineObject);

    cmd.bind_viewport(swapchain.m_extent);
    cmd.bind_scissor(swapchain.m_extent);

    auto view = nijiEngine.ecs.m_registry.view<Transform, MeshComponent>();
    for (auto&& [entity, trans, mesh] : view.each())
    {
        auto& model = mesh.Model;
        auto& modelMesh = model->m_meshes[mesh.MeshID];
        auto& material = model->m_materials[mesh.MeshID];

        VkBuffer vertexBuffers[] = {modelMesh.m_vertexBuffer.Handle};
        VkDeviceSize offsets[] = {0};
        cmd.bind_vertex_buffer(0, 1, vertexBuffers, offsets);
        cmd.bind_index_buffer(modelMesh.m_indexBuffer.Handle, 0,
                              modelMesh.m_ushortIndices ? VK_INDEX_TYPE_UINT16
                                                        : VK_INDEX_TYPE_UINT32);

        // Globals - 0
        {
            cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.PipelineLayout, 0,
                                     1,
                                     &renderer.m_globalDescriptor.m_set[renderer.m_currentFrame]);
        }

        // Pass Set
        {
            m_passDescriptor.m_info.Bindings[0].Resource = &material.m_data[frameIndex];

            std::vector<VkWriteDescriptorSet> writes = {};
            std::vector<VkDescriptorBufferInfo> bufferInfos = {};
            std::vector<VkDescriptorImageInfo> imageInfos = {};

            writes.reserve(m_passDescriptor.m_info.Bindings.size());
            bufferInfos.reserve(m_passDescriptor.m_info.Bindings.size());
            imageInfos.reserve(m_passDescriptor.m_info.Bindings.size());

            m_passDescriptor.push_descriptor_writes(writes, bufferInfos, imageInfos);

            cmd.push_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.PipelineLayout, 1,
                                    static_cast<uint32_t>(writes.size()), writes.data());
        }

        cmd.draw_indexed(static_cast<uint32_t>(modelMesh.m_indexCount), 1, 0, 0, 0);
    }

    cmd.end_rendering(info);
}

void DepthPass::cleanup()
{
    base_cleanup();
}