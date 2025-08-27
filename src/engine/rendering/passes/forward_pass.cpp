#include "forward_pass.hpp"

#include <functional>
#include <stdexcept>

#include <imgui_impl_vulkan.h>
#include <vk_mem_alloc.h>
#include <imgui.h>

#include "../../core/components/render-components.hpp"
#include "../../core/components/transform.hpp"
#include "../../engine.hpp"

using namespace niji;

void ForwardPass::init(Swapchain& swapchain, Descriptor& globalDescriptor)
{
    m_name = "Forward Pass";

    // Create Pass Data Buffer
    {
        VkDeviceSize bufferSize = sizeof(DebugSettings);
        m_passBuffer.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            DebugSettings ubo = {};
            BufferDesc bufferDesc = {};
            bufferDesc.IsPersistent = true;
            bufferDesc.Name = "Forward Pass Data";
            bufferDesc.Size = sizeof(DebugSettings);
            bufferDesc.Usage = BufferDesc::BufferUsage::Uniform;
            m_passBuffer[i] = Buffer(bufferDesc, &ubo);
        }
    }

    // Create Point Light Buffer
    {
        VkDeviceSize bufferSize = sizeof(PointLight);
        m_pointLightBuffer.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            std::array<PointLight, MAX_POINT_LIGHTS> buffer = {};
            BufferDesc bufferDesc = {};
            bufferDesc.IsPersistent = true;
            bufferDesc.Name = "Directional Lights Data";
            bufferDesc.Size = sizeof(PointLight) * MAX_POINT_LIGHTS;
            bufferDesc.Usage = BufferDesc::BufferUsage::Storage;
            m_pointLightBuffer[i] = Buffer(bufferDesc, nullptr);
        }
    }

    // Create Point Sampler
    {
        SamplerDesc desc = {};
        desc.MagFilter = SamplerDesc::Filter::NEAREST;
        desc.MinFilter = SamplerDesc::Filter::NEAREST;
        desc.AddressModeU = SamplerDesc::AddressMode::EDGE_CLAMP;
        desc.AddressModeV = SamplerDesc::AddressMode::EDGE_CLAMP;
        desc.AddressModeW = SamplerDesc::AddressMode::EDGE_CLAMP;
        desc.EnableAnisotropy = true;
        desc.MipmapMode = SamplerDesc::MipMapMode::NEAREST;
        desc.Name = "Forward Pass Point Sampler";

        m_pointSampler = Sampler(desc);
    }

    {
        DescriptorInfo descriptorInfo = {};
        descriptorInfo.IsPushDescriptor = true;
        descriptorInfo.Name = "Forward Pass Descriptor";

        DescriptorBinding passDataBinding = {};
        passDataBinding.Type = DescriptorBinding::BindType::UBO;
        passDataBinding.Count = 1;
        passDataBinding.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
        passDataBinding.Sampler = nullptr;
        passDataBinding.Resource = &m_passBuffer;
        descriptorInfo.Bindings.push_back(passDataBinding);

        DescriptorBinding pointLightBinding = {};
        pointLightBinding.Type = DescriptorBinding::BindType::STORAGE_BUFFER;
        pointLightBinding.Count = 1;
        pointLightBinding.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
        pointLightBinding.Sampler = nullptr;
        pointLightBinding.Resource = &m_pointLightBuffer;
        descriptorInfo.Bindings.push_back(pointLightBinding);

        DescriptorBinding materialBinding = {};
        materialBinding.Type = DescriptorBinding::BindType::UBO;
        materialBinding.Count = 1;
        materialBinding.Stage = DescriptorBinding::BindStage::ALL_GRAPHICS;
        materialBinding.Sampler = nullptr;
        descriptorInfo.Bindings.push_back(materialBinding);

        DescriptorBinding samplerBinding = {};
        samplerBinding.Type = DescriptorBinding::BindType::SAMPLER;
        samplerBinding.Count = 1;
        samplerBinding.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
        samplerBinding.Sampler = nullptr;
        descriptorInfo.Bindings.push_back(samplerBinding);

        DescriptorBinding samplerBinding2 = {};
        samplerBinding2.Type = DescriptorBinding::BindType::SAMPLER;
        samplerBinding2.Count = 1;
        samplerBinding2.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
        samplerBinding2.Sampler = nullptr;
        descriptorInfo.Bindings.push_back(samplerBinding2);

        // bindings 3–7 = individual sampled images
        for (size_t i = 0; i < 5; ++i)
        {
            DescriptorBinding textureBinding = {};
            textureBinding.Type = DescriptorBinding::BindType::TEXTURE;
            textureBinding.Count = 1;
            textureBinding.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
            textureBinding.Sampler = nullptr;
            descriptorInfo.Bindings.push_back(textureBinding);
        }
        // bindings 8-10 = IBL Images (specular, diffuse, brdf LUT)
        for (size_t i = 0; i < 3; ++i)
        {
            DescriptorBinding textureBinding = {};
            textureBinding.Type = DescriptorBinding::BindType::TEXTURE;
            textureBinding.Count = 1;
            textureBinding.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
            textureBinding.Sampler = nullptr;
            descriptorInfo.Bindings.push_back(textureBinding);
        }

        DescriptorBinding sceneInfoBinding = {};
        sceneInfoBinding.Type = DescriptorBinding::BindType::UBO;
        sceneInfoBinding.Count = 1;
        sceneInfoBinding.Stage = DescriptorBinding::BindStage::ALL;
        sceneInfoBinding.Sampler = nullptr;
        // sceneInfoBinding.Resource = &m_sceneInfoBuffer;
        descriptorInfo.Bindings.push_back(sceneInfoBinding);

        DescriptorBinding lightGridBinding = {};
        lightGridBinding.Type = DescriptorBinding::BindType::STORAGE_TEXTURE;
        lightGridBinding.Count = 1;
        lightGridBinding.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
        lightGridBinding.Sampler = nullptr;
        descriptorInfo.Bindings.push_back(lightGridBinding);

        DescriptorBinding lightIndexListBinding = {};
        lightIndexListBinding.Type = DescriptorBinding::BindType::STORAGE_BUFFER;
        lightIndexListBinding.Count = 1;
        lightIndexListBinding.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
        lightIndexListBinding.Sampler = nullptr;
        // lightIndexListBinding.Resource = &m_lightIndexList;
        descriptorInfo.Bindings.push_back(lightIndexListBinding);

        DescriptorBinding samplerBinding3 = {};
        samplerBinding3.Type = DescriptorBinding::BindType::SAMPLER;
        samplerBinding3.Count = 1;
        samplerBinding3.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
        samplerBinding3.Sampler = nullptr;
        descriptorInfo.Bindings.push_back(samplerBinding3);

        m_passDescriptor = Descriptor(descriptorInfo);
    }

    GraphicsPipelineDesc pipelineDesc = {globalDescriptor.m_setLayout,
                                         m_passDescriptor.m_setLayout};

    add_shader("shaders/forward_pass.slang", ShaderType::FRAG_AND_VERT);
    pipelineDesc.Name = "Forward Pass";
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
    pipelineDesc.DepthWriteEnable = false;
    pipelineDesc.DepthCompareOperation = GraphicsPipelineDesc::DepthCompareOp::LESS_OR_EQUAL;

    pipelineDesc.ColorAttachmentFormat = swapchain.m_format;

    pipelineDesc.VertexLayout =
        DEFINE_VERTEX_LAYOUT(Vertex,
                             VertexElement(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Pos)),
                             VertexElement(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Color)),
                             VertexElement(2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal)),
                             VertexElement(3, VK_FORMAT_R32G32B32A32_SFLOAT,
                                           offsetof(Vertex, Tangent)),
                             VertexElement(4, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, TexCoord)));

    m_pipelines.emplace(pipelineDesc.Name, Pipeline(pipelineDesc));

    nijiEngine.m_editor.add_debug_menu_panel("Forward Pass Panel", std::bind(&ForwardPass::debug_panel, this));
}

void ForwardPass::debug_panel()
{
    int currentIndex = static_cast<int>(m_debugSettings.RenderMode);

    if (ImGui::BeginCombo("Render Mode", RenderFlagNames[currentIndex]))
    {
        for (int i = 0; i < static_cast<int>(DebugSettings::RenderFlags::COUNT); ++i)
        {
            bool isSelected = (i == currentIndex);
            if (ImGui::Selectable(RenderFlagNames[i], isSelected))
            {
                m_debugSettings.RenderMode = static_cast<DebugSettings::RenderFlags>(i);
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::Checkbox("Draw Light Heatmap", &m_debugSettings.DrawLightHeatmap);
}

void ForwardPass::update_impl(Renderer& renderer, CommandList& cmd)
{
    const uint32_t& frameIndex = renderer.m_currentFrame;

    /*static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration<float, std::chrono::seconds::period>(currentTime -
    startTime).count();*/

    {
        memcpy(m_passBuffer[frameIndex].Data, &m_debugSettings, sizeof(m_debugSettings));

        vkCmdUpdateBuffer(cmd.m_commandBuffer, m_passBuffer[frameIndex].Handle, 0,
                          sizeof(DebugSettings), &m_debugSettings);
    }

    {
        std::vector<PointLight> pointLightsArray = {};

        auto pointLightView = nijiEngine.ecs.m_registry.view<PointLight>();
        for (const auto& [ent, pointLight] : pointLightView.each())
        {
            if (pointLightsArray.size() < MAX_POINT_LIGHTS)
                pointLightsArray.push_back(pointLight);
            else
                printf("\nWARNING: Max Amount of Point Lights Reached!\n");
        }

        vkCmdUpdateBuffer(cmd.m_commandBuffer, m_pointLightBuffer[frameIndex].Handle, 0,
                          sizeof(PointLight) * pointLightsArray.size(), pointLightsArray.data());
    }
}

void ForwardPass::record(Renderer& renderer, CommandList& cmd, RenderInfo& info)
{
    Swapchain& swapchain = renderer.m_swapchain;
    const uint32_t& frameIndex = renderer.m_currentFrame;
    const Pipeline& pipeline = m_pipelines.at("Forward Pass"); 

    info.ViewportTarget->StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    info.ViewportTarget->LoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

    info.DepthAttachment->StoreOp = VK_ATTACHMENT_STORE_OP_NONE;
    info.DepthAttachment->LoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

    {
        TransitionInfo before = {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                                 VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL};

        TransitionInfo after = {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (nijiEngine.m_context.has_stencil_component(info.DepthAttachment->Format))
            aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;

        cmd.transition_image_explicit(*info.DepthAttachment, before, after, aspect, 1, 1);
    }

    //// DEBUG ONLY
    //{
    //    VkMemoryBarrier2 memoryBarrier = {};
    //    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    //    memoryBarrier.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    //    memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    //    memoryBarrier.dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    //    memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

    //    VkDependencyInfo depInfo = {};
    //    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    //    depInfo.memoryBarrierCount = 1;
    //    depInfo.pMemoryBarriers = &memoryBarrier;

    //    vkCmdPipelineBarrier2(cmd.m_commandBuffer, &depInfo);
    //}

    cmd.begin_rendering(info, m_name);

    cmd.bind_pipeline(pipeline.PipelineObject);

    cmd.bind_viewport(swapchain.m_extent);
    cmd.bind_scissor(swapchain.m_extent);

    static bool b = true;
    ImGui::ShowMetricsWindow(&b);

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

        // Per-Pass - 1
        {
            m_passDescriptor.m_info.Bindings[0].Resource = &m_passBuffer[frameIndex];

            m_passDescriptor.m_info.Bindings[1].Resource = &m_pointLightBuffer[frameIndex];

            m_passDescriptor.m_info.Bindings[2].Resource = &material.m_data[frameIndex];

            m_passDescriptor.m_info.Bindings[3].Resource = &material.m_sampler;

            m_passDescriptor.m_info.Bindings[4].Resource = &renderer.m_envmap->m_sampler;

            std::array<std::optional<Texture>*, 5> textures = {
                &material.m_materialData.BaseColor, &material.m_materialData.NormalTexture,
                &material.m_materialData.OcclusionTexture, &material.m_materialData.RoughMetallic,
                &material.m_materialData.Emissive};

            // Model Textures (binding 3..7)
            for (size_t i = 0; i < 5; ++i)
            {
                bool hasValue = textures[i]->has_value();
                m_passDescriptor.m_info.Bindings[5 + i].Resource =
                    hasValue ? &(textures[i]->value()) : &renderer.m_fallbackTexture;
            }

            // m_depthTexture = Texture(*info.DepthAttachment);
            // m_passDescriptor.m_info.Bindings[5].Resource = &m_depthTexture;

            // IBL Textures (binding 8..10)
            if (renderer.m_envmap)
            {
                m_passDescriptor.m_info.Bindings[10 + 0].Resource =
                    &renderer.m_envmap->m_specularCubemap;
                m_passDescriptor.m_info.Bindings[10 + 1].Resource =
                    &renderer.m_envmap->m_diffuseCubemap;
                m_passDescriptor.m_info.Bindings[10 + 2].Resource =
                    &renderer.m_envmap->m_brdfTexture;
            }
            else
            {
                printf("\nWARNING: Envmap is Null! \n");
                m_passDescriptor.m_info.Bindings[10 + 0].Resource = &renderer.m_fallbackTexture;
                m_passDescriptor.m_info.Bindings[10 + 1].Resource = &renderer.m_fallbackTexture;
                m_passDescriptor.m_info.Bindings[10 + 2].Resource = &renderer.m_fallbackTexture;
            }
            m_passDescriptor.m_info.Bindings[13].Resource = &renderer.m_sceneInfoBuffer[frameIndex];

            m_passDescriptor.m_info.Bindings[14].Resource = &renderer.m_lightGridTexture;
            m_passDescriptor.m_info.Bindings[15].Resource = &renderer.m_lightIndexList[frameIndex];
            m_passDescriptor.m_info.Bindings[16].Resource = &m_pointSampler;

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

void ForwardPass::cleanup()
{
    base_cleanup();

    m_pointSampler.cleanup();

    for (int i = 0; i < m_pointLightBuffer.size(); i++)
    {
        m_pointLightBuffer[i].cleanup();
    }
}