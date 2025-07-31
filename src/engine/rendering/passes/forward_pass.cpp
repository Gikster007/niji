#include "forward_pass.hpp"

#include <stdexcept>

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

    // Create Scene Info Buffer
    {
        VkDeviceSize bufferSize = sizeof(SceneInfo);
        m_sceneInfoBuffer.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            SceneInfo ubo = {};
            BufferDesc bufferDesc = {};
            bufferDesc.IsPersistent = true;
            bufferDesc.Name = "Scene Info Data";
            bufferDesc.Size = sizeof(SceneInfo);
            bufferDesc.Usage = BufferDesc::BufferUsage::Uniform;
            m_sceneInfoBuffer[i] = Buffer(bufferDesc, &ubo);
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

    {
        DescriptorInfo descriptorInfo = {};
        descriptorInfo.IsPushDescriptor = true;

        DescriptorBinding passDataBinding = {};
        passDataBinding.Type = DescriptorBinding::BindType::UBO;
        passDataBinding.Count = 1;
        passDataBinding.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
        passDataBinding.Sampler = nullptr;
        passDataBinding.Resource = &m_passBuffer;
        descriptorInfo.Bindings.push_back(passDataBinding);

        DescriptorBinding dirLightBinding = {};
        dirLightBinding.Type = DescriptorBinding::BindType::UBO;
        dirLightBinding.Count = 1;
        dirLightBinding.Stage = DescriptorBinding::BindStage::FRAGMENT_SHADER;
        dirLightBinding.Sampler = nullptr;
        dirLightBinding.Resource = &m_sceneInfoBuffer;
        descriptorInfo.Bindings.push_back(dirLightBinding);

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

        m_passDescriptor = Descriptor(descriptorInfo);
    }

    PipelineDesc pipelineDesc = {globalDescriptor.m_setLayout, m_passDescriptor.m_setLayout};

    pipelineDesc.Name = "Forward Pass";
    pipelineDesc.VertexShader = "shaders/spirv/forward_pass.vert.spv";
    pipelineDesc.FragmentShader = "shaders/spirv/forward_pass.frag.spv";

    pipelineDesc.Rasterizer.CullMode = RasterizerState::CullingMode::BACK;
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
        DEFINE_VERTEX_LAYOUT(Vertex,
                             VertexElement(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Pos)),
                             VertexElement(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Color)),
                             VertexElement(2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal)),
                             VertexElement(3, VK_FORMAT_R32G32B32A32_SFLOAT,
                                           offsetof(Vertex, Tangent)),
                             VertexElement(4, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, TexCoord)));

    m_pipeline = Pipeline(pipelineDesc);
}

void ForwardPass::update(Renderer& renderer)
{
    const uint32_t& frameIndex = renderer.m_currentFrame;

    /*static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration<float, std::chrono::seconds::period>(currentTime -
    startTime).count();*/

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
        memcpy(m_passBuffer[frameIndex].Data, &m_debugSettings, sizeof(m_debugSettings));
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
        memcpy(m_pointLightBuffer[frameIndex].Data, pointLightsArray.data(),
               sizeof(PointLight) * pointLightsArray.size());

        auto dirLightView = nijiEngine.ecs.m_registry.view<DirectionalLight>();
        for (const auto& [ent, dirLight] : dirLightView.each())
        {
            SceneInfo sceneInfo = {};
            sceneInfo.DirLight = dirLight;
            sceneInfo.PointLightCount = pointLightsArray.size();

            memcpy(m_sceneInfoBuffer[frameIndex].Data, &sceneInfo, sizeof(sceneInfo));
        }
    }

    {
        ModelData ubo = {};

        auto view = nijiEngine.ecs.m_registry.view<Transform, MeshComponent>();
        for (auto&& [entity, trans, mesh] : view.each())
        {
            auto& model = mesh.Model;
            auto& modelMesh = model->m_meshes[mesh.MeshID];
            auto& material = model->m_materials[mesh.MaterialID];

            /*ubo.Model = glm::rotate(trans.World(), time * glm::radians(30.0f), glm::vec3(0.0f,
            0.0f, 1.0f));*/
            ubo.Model = trans.World();
            ubo.InvModel = glm::transpose(glm::inverse(ubo.Model));

            ubo.MaterialInfo = material.m_materialInfo;

            memcpy(material.m_data[frameIndex].Data, &ubo, sizeof(ubo));
        }
    }
}

void ForwardPass::record(Renderer& renderer, CommandList& cmd, RenderInfo& info)
{
    Swapchain& swapchain = renderer.m_swapchain;
    const uint32_t& frameIndex = renderer.m_currentFrame;

    info.ColorAttachment.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    info.ColorAttachment.LoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

    cmd.begin_rendering(info, m_name);

    cmd.bind_pipeline(m_pipeline.PipelineObject);

    cmd.bind_viewport(swapchain.m_extent);
    cmd.bind_scissor(swapchain.m_extent);

    ImGui::Begin("Demo Window");
    static bool b = true;
    ImGui::ShowDemoWindow(&b);
    ImGui::ShowMetricsWindow(&b);
    ImGui::Text("Hello, Vulkan + ImGui!");
    ImGui::End();

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
                                     1,
                                     &renderer.m_globalDescriptor.m_set[renderer.m_currentFrame]);
        }

        // Per-Pass - 1
        {
            m_passDescriptor.m_info.Bindings[0].Resource = &m_passBuffer[frameIndex];

            m_passDescriptor.m_info.Bindings[1].Resource = &m_sceneInfoBuffer[frameIndex];

            m_passDescriptor.m_info.Bindings[2].Resource = &m_pointLightBuffer[frameIndex];

            m_passDescriptor.m_info.Bindings[3].Resource = &material.m_data[frameIndex];

            m_passDescriptor.m_info.Bindings[4].Resource = &material.m_sampler;

            m_passDescriptor.m_info.Bindings[5].Resource = &renderer.m_envmap->m_sampler;

            std::array<std::optional<Texture>*, 5> textures = {
                &material.m_materialData.BaseColor, &material.m_materialData.NormalTexture,
                &material.m_materialData.OcclusionTexture, &material.m_materialData.RoughMetallic,
                &material.m_materialData.Emissive};

            // Model Textures (binding 3..7)
            for (size_t i = 0; i < 5; ++i)
            {
                bool hasValue = textures[i]->has_value();
                m_passDescriptor.m_info.Bindings[6 + i].Resource =
                    hasValue ? &(textures[i]->value()) : &renderer.m_fallbackTexture;
            }
            // IBL Textures (binding 8..10)
            if (renderer.m_envmap)
            {
                m_passDescriptor.m_info.Bindings[11 + 0].Resource =
                    &renderer.m_envmap->m_specularCubemap;
                m_passDescriptor.m_info.Bindings[11 + 1].Resource =
                    &renderer.m_envmap->m_diffuseCubemap;
                m_passDescriptor.m_info.Bindings[11 + 2].Resource =
                    &renderer.m_envmap->m_brdfTexture;
            }
            else
            {
                printf("\nWARNING: Envmap is Null! \n");
                m_passDescriptor.m_info.Bindings[11 + 0].Resource = &renderer.m_fallbackTexture;
                m_passDescriptor.m_info.Bindings[11 + 1].Resource = &renderer.m_fallbackTexture;
                m_passDescriptor.m_info.Bindings[11 + 2].Resource = &renderer.m_fallbackTexture;
            }

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

        cmd.draw_indexed(static_cast<uint32_t>(modelMesh.m_indexCount), 1, 0, 0, 0);
    }

    cmd.end_rendering(info);
}

void ForwardPass::cleanup()
{
    base_cleanup();

    for (int i = 0; i < m_sceneInfoBuffer.size(); i++)
    {
        m_sceneInfoBuffer[i].cleanup();
    }

    for (int i = 0; i < m_pointLightBuffer.size(); i++)
    {
        m_pointLightBuffer[i].cleanup();
    }
}