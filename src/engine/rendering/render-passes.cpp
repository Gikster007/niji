#include "render-passes.hpp"

#include <stdexcept>

#include "../engine.hpp"
#include "../core/components/transform.hpp"
#include "../core/components/render-components.hpp"

using namespace niji;

void ForwardPass::init(VkExtent2D swapChainExtent, VkDescriptorSetLayout& globalLayout)
{
    // Create Pass Data Buffer
    VkDeviceSize bufferSize = sizeof(RenderFlags);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        RenderFlags ubo = {};
        BufferDesc bufferDesc = {};
        bufferDesc.IsPersistent = true;
        bufferDesc.Name = "Forward Pass Data";
        bufferDesc.Size = sizeof(RenderFlags);
        bufferDesc.Usage = BufferDesc::BufferUsage::Uniform;
        m_passBuffer[i] = Buffer(bufferDesc, &ubo);
    }

    {
        std::array<VkDescriptorSetLayoutBinding, 4> bindings = {};
        bindings[0] = {};
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[1] = {};
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        bindings[2] = {};
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[2].pImmutableSamplers = nullptr;

        bindings[3] = {};
        bindings[3].binding = 3;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[3].descriptorCount = 5;
        bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[3].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo pushLayoutInfo = {};
        pushLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        pushLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        pushLayoutInfo.bindingCount = bindings.size();
        pushLayoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(nijiEngine.m_context.m_device, &pushLayoutInfo, nullptr,
                                        &m_passDescriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Forward Pass push descriptor set layout");
        }
    }

    PipelineDesc pipelineDesc = {globalLayout, m_passDescriptorSetLayout};

    pipelineDesc.Name = "Forward Pass";
    pipelineDesc.VertexShader = "shaders/vert.spv";
    pipelineDesc.FragmentShader = "shaders/frag.spv";

    pipelineDesc.Rasterizer.CullMode = RasterizerState::CullingMode::BACK;
    pipelineDesc.Rasterizer.PolyMode = RasterizerState::PolygonMode::FILL;
    pipelineDesc.Rasterizer.RasterizerDiscardEnable = false;
    pipelineDesc.Rasterizer.DepthClampEnable = false;
    pipelineDesc.Rasterizer.LineWidth = 1.0f;

    pipelineDesc.Topology = PipelineDesc::PrimitiveTopology::TRIANGLE_LIST;

    pipelineDesc.Viewport.Width = swapChainExtent.width;
    pipelineDesc.Viewport.Height = swapChainExtent.height;
    pipelineDesc.Viewport.MaxDepth = 1.0f;
    pipelineDesc.Viewport.MinDepth = 0.0f;

    pipelineDesc.VertexLayout =
        DEFINE_VERTEX_LAYOUT(Vertex,
                             VertexElement(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Pos)),
                             VertexElement(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Color)),
                             VertexElement(2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal)),
                             VertexElement(3, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, TexCoord)));

    m_pipeline = Pipeline(pipelineDesc);
}

void ForwardPass::update(Renderer& renderer)
{
    const uint32_t frameIndex = renderer.m_currentFrame;

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    {
        RenderFlags flags = {};
        memcpy(m_passBuffer[frameIndex].Data, &flags, sizeof(flags));
    }

    {
        ModelData ubo = {};

        auto view = nijiEngine.ecs.m_registry.view<Transform, MeshComponent>();
        for (auto&& [entity, trans, mesh] : view.each())
        {
            auto& model = mesh.Model;
            auto& modelMesh = model->m_meshes[mesh.MeshID];
            auto& material = model->m_materials[mesh.MaterialID];

            ubo.Model =
                glm::rotate(trans.World(), time * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));

            memcpy(material.m_data[frameIndex].Data, &ubo, sizeof(ubo));
        }
    }
}

void ForwardPass::record(Renderer& renderer, CommandList& cmd)
{
    const uint32_t frameIndex = renderer.m_currentFrame;

    cmd.reset();

    cmd.begin_list("Forward Pass");

    RenderTarget colorAttachment = {renderer.m_swapChainImages[renderer.m_imageIndex],
                                    renderer.m_swapChainImageViews[renderer.m_imageIndex]};
    colorAttachment.ClearValue = {0.0f, 0.0f, 0.0f, 1.0f};
    VkFormat depthFormat = nijiEngine.m_context.find_depth_format();
    RenderTarget depthStencil = {renderer.m_depthImage, renderer.m_depthImageView, depthFormat};
    depthStencil.ClearValue = {1.0f, 0.0f};

    RenderInfo info = {renderer.m_swapChainExtent};
    info.ColorAttachments.push_back(colorAttachment);
    info.DepthAttachment = depthStencil;
    info.HasDepth = true;

    cmd.begin_rendering(info);

    cmd.bind_pipeline(m_pipeline.PipelineObject);

    /*VkViewport viewport = {};
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
    cmd.bind_scissor(scissor);*/

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
            cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.PipelineLayout, 0,
                                     1, &renderer.m_globalDescriptorSet[renderer.m_currentFrame]);
        }

        // Per-Pass - 1
        {
            std::vector<VkWriteDescriptorSet> writes = {};

            // UBO
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = m_passBuffer[frameIndex].Handle;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(RenderFlags);

            VkWriteDescriptorSet uboWrite = {};
            uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uboWrite.dstBinding = 0;
            uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWrite.descriptorCount = 1;
            uboWrite.pBufferInfo = &bufferInfo;
            writes.push_back(uboWrite);

            // UBO - Material
            VkDescriptorBufferInfo matBufferInfo = {};
            matBufferInfo.buffer = material.m_data[frameIndex].Handle;
            matBufferInfo.offset = 0;
            matBufferInfo.range = sizeof(ModelData);

            VkWriteDescriptorSet matWrite = {};
            matWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            matWrite.dstBinding = 1;
            matWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            matWrite.descriptorCount = 1;
            matWrite.pBufferInfo = &matBufferInfo;
            writes.push_back(matWrite);

            // Sampler
            VkDescriptorImageInfo samplerInfo = {};
            samplerInfo.sampler = material.m_sampler;

            VkWriteDescriptorSet samplerWrite = {};
            samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            samplerWrite.dstBinding = 2;
            samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            samplerWrite.descriptorCount = 1;
            samplerWrite.pImageInfo = &samplerInfo;
            writes.push_back(samplerWrite);

            // Sampled Images (up to 5)
            std::vector<VkDescriptorImageInfo> imageInfos;
            std::vector<std::optional<NijiTexture>> textures = {
                material.m_materialData.BaseColor,        material.m_materialData.NormalTexture,
                material.m_materialData.OcclusionTexture, material.m_materialData.RoughMetallic,
                material.m_materialData.Emissive,
            };

            for (auto& tex : textures)
            {
                // if (tex.has_value())
                imageInfos.push_back(tex->ImageInfo);
                // else
                //{
                //     // Fallback black texture?
                //     VkDescriptorImageInfo dummy = {};
                //     dummy.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                //     dummy.imageView = renderer.getBlackImageView();
                //     dummy.sampler = VK_NULL_HANDLE;
                //     imageInfos.push_back(dummy);
                // }
            }

            VkWriteDescriptorSet imageWrite = {};
            imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageWrite.dstBinding = 3;
            imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            imageWrite.descriptorCount = static_cast<uint32_t>(imageInfos.size());
            imageWrite.pImageInfo = imageInfos.data();
            writes.push_back(imageWrite);

            // Push Descriptor Set
            cmd.push_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.PipelineLayout, 1,
                                    static_cast<uint32_t>(writes.size()), writes.data());
        }

        cmd.draw_indexed(static_cast<uint32_t>(modelMesh.m_indexCount), 1, 0, 0, 0);
    }

    cmd.end_rendering(info);
    cmd.end_list();
}
