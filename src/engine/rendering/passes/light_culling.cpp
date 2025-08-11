#include "light_culling.hpp"

#include <vk_mem_alloc.h>

#include "../app/camera_system.hpp"

#include "rendering/renderer.hpp"
#include "core/vulkan-functions.hpp"
#include "engine.hpp"
#include <imgui.h>

using namespace niji;

void LightCullingPass::init(Swapchain& swapchain, Descriptor& globalDescriptor)
{
    m_name = "Light Culling Pass";

    // Init "m_totalThreads" and "m_totalThreadGroups"
    on_window_resize();

    // Create Dispatch Param Buffers
    {
        VkDeviceSize bufferSize = sizeof(DispatchParams);
        m_dispatchParams.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            DispatchParams ubo = {};
            BufferDesc bufferDesc = {};
            bufferDesc.IsPersistent = true;
            bufferDesc.Name = "Dispatch Params Data";
            bufferDesc.Size = sizeof(DispatchParams);
            bufferDesc.Usage = BufferDesc::BufferUsage::Uniform;
            m_dispatchParams[i] = Buffer(bufferDesc, &ubo);
        }
    }

    // Create Frustums Buffer
    {
        VkDeviceSize bufferSize = sizeof(Frustum);
        m_frustums.resize(MAX_FRAMES_IN_FLIGHT);
        const uint32_t totalTiles = m_totalThreads.x * m_totalThreads.y;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            std::vector<Frustum> buffer = {};
            buffer.resize(totalTiles);
            BufferDesc bufferDesc = {};
            bufferDesc.IsPersistent = true;
            bufferDesc.Name = "Tile Frustums";
            bufferDesc.Size = sizeof(Frustum) * totalTiles;
            bufferDesc.Usage = BufferDesc::BufferUsage::Storage;
            m_frustums[i] = Buffer(bufferDesc, buffer.data());
        }
    }

    //// Set = 1, Binding = 2
    //[[vk::binding(2, 1)]]
    // RWStructuredBuffer<uint> o_LightIndexCounter; DONE
    //// Set = 1, Binding = 3
    //[[vk::binding(3, 1)]]
    // RWStructuredBuffer<uint> o_LightIndexList;
    //// Set = 1, Binding = 4
    //[[vk::binding(4, 1)]]
    // RWTexture2D<uint2> o_LightGrid;

    // Create Light Index Counter Buffer
    {
        VkDeviceSize bufferSize = sizeof(LightIndexCounter);
        m_lightIndexCounter.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            LightIndexCounter buffer = {};
            BufferDesc bufferDesc = {};
            bufferDesc.IsPersistent = true;
            bufferDesc.Name = "Light Index Counter Buffer";
            bufferDesc.Size = sizeof(LightIndexCounter);
            bufferDesc.Usage = BufferDesc::BufferUsage::Storage;
            m_lightIndexCounter[i] = Buffer(bufferDesc, &buffer);
        }
    }

    //// Create Light Index List Buffer
    //{
    //    VkDeviceSize bufferSize = sizeof(LightIndexList);
    //    m_lightIndexList.resize(MAX_FRAMES_IN_FLIGHT);
    //    const uint32_t totalTiles = m_totalThreads.x * m_totalThreads.y;
    //    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    //    {
    //        LightIndexList buffer = {};
    //        buffer.counter = 0;
    //        BufferDesc bufferDesc = {};
    //        bufferDesc.IsPersistent = true;
    //        bufferDesc.Name = "Light Index List Buffer";
    //        bufferDesc.Size = sizeof(LightIndexList) * totalTiles * MAX_LIGHTS_PER_TILE;
    //        bufferDesc.Usage = BufferDesc::BufferUsage::Storage;
    //        m_lightIndexList[i] = Buffer(bufferDesc, &buffer);
    //    }
    //}

    //// Create Light Grid RWTexture
    //{
    //    TextureDesc desc = {};
    //    desc.Width = m_totalThreads.x;
    //    desc.Height = m_totalThreads.y;
    //    desc.Channels = 2;
    //    desc.IsMipMapped = false;
    //    desc.Data = nullptr;
    //    desc.Format = VK_FORMAT_R32G32_UINT;
    //    desc.MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    //    desc.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    //    desc.IsReadWrite = true;
    //    m_lightGridTexture = Texture(desc);
    //}

    // Init Push Descriptor
    {
        DescriptorInfo descriptorInfo = {};
        descriptorInfo.IsPushDescriptor = true;
        descriptorInfo.Name = "Light Culling Pass Descriptor";

        DescriptorBinding passParamsBinding = {};
        passParamsBinding.Type = DescriptorBinding::BindType::UBO;
        passParamsBinding.Count = 1;
        passParamsBinding.Stage = DescriptorBinding::BindStage::COMPUTE;
        passParamsBinding.Sampler = nullptr;
        passParamsBinding.Resource = &m_dispatchParams;
        descriptorInfo.Bindings.push_back(passParamsBinding);

        DescriptorBinding frustumsBinding = {};
        frustumsBinding.Type = DescriptorBinding::BindType::STORAGE_BUFFER;
        frustumsBinding.Count = 1;
        frustumsBinding.Stage = DescriptorBinding::BindStage::COMPUTE;
        frustumsBinding.Sampler = nullptr;
        frustumsBinding.Resource = &m_frustums;
        descriptorInfo.Bindings.push_back(frustumsBinding);

        m_passDescriptor = Descriptor(descriptorInfo);
    }

    // Grid Frustums Pipeline
    {
        ComputePipelineDesc gridFrustumsDesc = {globalDescriptor.m_setLayout,
                                                m_passDescriptor.m_setLayout};
        gridFrustumsDesc.Name = "Grid Frustum Compute Pass";
        gridFrustumsDesc.ComputeShader = "shaders/spirv/grid_frustums_cs.spv";

        m_pipeline = Pipeline(gridFrustumsDesc);
    }

    // Init Light Culling Descriptor
    {
        DescriptorInfo descriptorInfo = {};
        descriptorInfo.IsPushDescriptor = true;
        descriptorInfo.Name = "Light Culling Pass Descriptor";

        DescriptorBinding passParamsBinding = {};
        passParamsBinding.Type = DescriptorBinding::BindType::UBO;
        passParamsBinding.Count = 1;
        passParamsBinding.Stage = DescriptorBinding::BindStage::COMPUTE;
        passParamsBinding.Sampler = nullptr;
        passParamsBinding.Resource = &m_dispatchParams;
        descriptorInfo.Bindings.push_back(passParamsBinding);

        DescriptorBinding frustumsBinding = {};
        frustumsBinding.Type = DescriptorBinding::BindType::STORAGE_BUFFER;
        frustumsBinding.Count = 1;
        frustumsBinding.Stage = DescriptorBinding::BindStage::COMPUTE;
        frustumsBinding.Sampler = nullptr;
        frustumsBinding.Resource = &m_frustums;
        descriptorInfo.Bindings.push_back(frustumsBinding);

        DescriptorBinding lightIndexCounterBinding = {};
        lightIndexCounterBinding.Type = DescriptorBinding::BindType::STORAGE_BUFFER;
        lightIndexCounterBinding.Count = 1;
        lightIndexCounterBinding.Stage = DescriptorBinding::BindStage::COMPUTE;
        lightIndexCounterBinding.Sampler = nullptr;
        lightIndexCounterBinding.Resource = &m_lightIndexCounter;
        descriptorInfo.Bindings.push_back(lightIndexCounterBinding);

        DescriptorBinding lightIndexListBinding = {};
        lightIndexListBinding.Type = DescriptorBinding::BindType::STORAGE_BUFFER;
        lightIndexListBinding.Count = 1;
        lightIndexListBinding.Stage = DescriptorBinding::BindStage::COMPUTE;
        lightIndexListBinding.Sampler = nullptr;
        // lightIndexListBinding.Resource = &m_lightIndexList;
        descriptorInfo.Bindings.push_back(lightIndexListBinding);

        DescriptorBinding lightGridTextureBinding = {};
        lightGridTextureBinding.Type = DescriptorBinding::BindType::STORAGE_TEXTURE;
        lightGridTextureBinding.Count = 1;
        lightGridTextureBinding.Stage = DescriptorBinding::BindStage::COMPUTE;
        lightGridTextureBinding.Sampler = nullptr;
        // lightGridTextureBinding.Resource = &m_lightGridTexture;
        descriptorInfo.Bindings.push_back(lightGridTextureBinding);

        DescriptorBinding depthTextureBinding = {};
        depthTextureBinding.Type = DescriptorBinding::BindType::TEXTURE;
        depthTextureBinding.Count = 1;
        depthTextureBinding.Stage = DescriptorBinding::BindStage::COMPUTE;
        depthTextureBinding.Sampler = nullptr;
        descriptorInfo.Bindings.push_back(depthTextureBinding);

        DescriptorBinding sphereLights2 = {};
        sphereLights2.Type = DescriptorBinding::BindType::STORAGE_BUFFER;
        sphereLights2.Count = 1;
        sphereLights2.Stage = DescriptorBinding::BindStage::ALL;
        sphereLights2.Sampler = nullptr;
        // sphereLights2.Resource = &m_spheres;
        descriptorInfo.Bindings.push_back(sphereLights2);

        DescriptorBinding sceneInfoBinding = {};
        sceneInfoBinding.Type = DescriptorBinding::BindType::UBO;
        sceneInfoBinding.Count = 1;
        sceneInfoBinding.Stage = DescriptorBinding::BindStage::ALL;
        sceneInfoBinding.Sampler = nullptr;
        // sceneInfoBinding.Resource = &m_sceneInfoBuffer;
        descriptorInfo.Bindings.push_back(sceneInfoBinding);

        m_lightCullingDescriptor = Descriptor(descriptorInfo);
    }

    // Light Culling Pipeline
    {
        ComputePipelineDesc lightCullingDesc = {globalDescriptor.m_setLayout,
                                                m_lightCullingDescriptor.m_setLayout};
        lightCullingDesc.Name = "Light Culling Compute Pass";
        lightCullingDesc.ComputeShader = "shaders/spirv/light_culling_cs.spv";

        m_lightCullingPipeline = Pipeline(lightCullingDesc);
    }
}

void LightCullingPass::update(Renderer& renderer, CommandList& cmd)
{
    on_window_resize();

    const uint32_t& frameIndex = renderer.m_currentFrame;

    {
        DispatchParams ubo = {};

        auto& cameraSystem = nijiEngine.ecs.find_system<CameraSystem>();
        auto& camera = cameraSystem.m_camera;

        ubo.InverseProjection = glm::inverse(camera.GetProjectionMatrix());
        auto text = ubo.InverseProjection * camera.GetProjectionMatrix();
        ubo.numThreadGroups = glm::u32vec3(m_totalThreadGroups, 1);
        ubo.numThreads = glm::u32vec3(m_totalThreads, 1);
        ubo.ScreenDimensions = {m_winWidth, m_winHeight};

        vkCmdUpdateBuffer(cmd.m_commandBuffer, m_dispatchParams[frameIndex].Handle, 0,
                          sizeof(DispatchParams), &ubo);
    }

    {
        uint32_t zero = 0;
        vkCmdUpdateBuffer(cmd.m_commandBuffer, m_lightIndexCounter[frameIndex].Handle, 0,
                          sizeof(uint32_t), &zero);
    }
}

glm::vec3 plane_intersection(const glm::vec3& n1, float d1, const glm::vec3& n2, float d2,
                             const glm::vec3& n3, float d3)
{
    glm::vec3 n2xn3 = glm::cross(n2, n3);
    glm::vec3 n3xn1 = glm::cross(n3, n1);
    glm::vec3 n1xn2 = glm::cross(n1, n2);
    float denom = glm::dot(n1, n2xn3);
    //assert(fabs(denom) > 1e-6f); // planes should not be parallel
    /*if (fabs(denom) < 1e-6f)
    {
        return glm::vec3(0.0f);
    }*/
    return (-d1 * n2xn3 - d2 * n3xn1 - d3 * n1xn2) / denom;
}

static bool computeFrustums = true;
static bool captureFrustum = false;
void LightCullingPass::record(Renderer& renderer, CommandList& cmd, RenderInfo& info)
{
    Swapchain& swapchain = renderer.m_swapchain;
    const uint32_t& frameIndex = renderer.m_currentFrame;

    info.DepthAttachment->StoreOp = VK_ATTACHMENT_STORE_OP_NONE;
    info.DepthAttachment->LoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

    {
        TransitionInfo before = {VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};

        TransitionInfo after = {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};

        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (nijiEngine.m_context.has_stencil_component(info.DepthAttachment->Format))
            aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;

        cmd.transition_image_explicit(*info.DepthAttachment, before, after, aspect, 1, 1);
    }

    if (computeFrustums)
    {
        // computeFrustums = false;
        //  Grid Frustums Computation
        {
            VkDebugUtilsLabelEXT labelInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
            labelInfo.pLabelName = "Compute Grid Frustums";
            labelInfo.color[0] = 0.2f;
            labelInfo.color[1] = 0.6f;
            labelInfo.color[2] = 0.9f;
            labelInfo.color[3] = 1.0f;

            VKCmdBeginDebugUtilsLabelEXT(cmd.m_commandBuffer, &labelInfo);

            cmd.bind_pipeline(m_pipeline.PipelineObject, true);

            // Per Pass Bindings
            {
                m_passDescriptor.m_info.Bindings[0].Resource = &m_dispatchParams[frameIndex];

                m_passDescriptor.m_info.Bindings[1].Resource = &m_frustums[frameIndex];

                std::vector<VkWriteDescriptorSet> writes = {};
                std::vector<VkDescriptorBufferInfo> bufferInfos = {};
                std::vector<VkDescriptorImageInfo> imageInfos = {};

                writes.reserve(m_passDescriptor.m_info.Bindings.size());
                bufferInfos.reserve(m_passDescriptor.m_info.Bindings.size());
                imageInfos.reserve(m_passDescriptor.m_info.Bindings.size());

                m_passDescriptor.push_descriptor_writes(writes, bufferInfos, imageInfos);

                cmd.push_descriptor_set(VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.PipelineLayout,
                                        1, static_cast<uint32_t>(writes.size()), writes.data());
            }

            cmd.dispatch(m_totalThreadGroups.x, m_totalThreadGroups.y, 1);

            VKCmdEndDebugUtilsLabelEXT(cmd.m_commandBuffer);

            // cmd.end_rendering(info);
        }

        // Syncing (for Frustums Buffer)
        {
            VkBufferMemoryBarrier2 memBarrier = {};
            memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
            memBarrier.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            memBarrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            memBarrier.buffer = renderer.m_lightIndexList[frameIndex].Handle;
            memBarrier.size = renderer.m_lightIndexList[frameIndex].Desc.Size;

            VkDependencyInfo depInfo = {};
            depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            depInfo.bufferMemoryBarrierCount = 1;
            depInfo.pBufferMemoryBarriers = &memBarrier;

            VKCmdPipelineBarrier2KHR(cmd.m_commandBuffer, &depInfo);
        }
    }

    // Debug Draw Frustum Planes
    {
        ImGui::Begin("Debug");
        ImGui::Checkbox("Debug Frustums", &captureFrustum);
        ImGui::End();
        if (captureFrustum)
        {
            glm::vec3 camPos = glm::vec3(0, 0, 0);
            float nearDist = 0.1f;
            float farDist = 1.0f;

            glm::vec3 nearCenter = glm::vec3(0, 0, -nearDist);
            glm::vec3 farCenter = glm::vec3(0, 0, -farDist);

            glm::vec3 nearNormal = glm::vec3(0, 0, 1); // looking down -Z
            float nearD = nearDist;                    // signed distance from origin (positive)

            glm::vec3 farNormal = glm::vec3(0, 0, -1);
            float farD = -farDist;

            Frustum* frustums = reinterpret_cast<Frustum*>(m_frustums[frameIndex].Data);
            for (int y = 0; y < m_totalThreads.y; y++)
            {
                for (int x = 0; x < m_totalThreads.x; x++)
                {

                    Frustum& frustum = frustums[x + y * m_totalThreads.x];

                    glm::vec3 leftPlaneNormal = frustums->Planes[0].Normal;
                    glm::vec3 rightPlaneNormal = frustums->Planes[1].Normal;
                    glm::vec3 topPlaneNormal = frustums->Planes[2].Normal;
                    glm::vec3 bottomPlaneNormal = frustums->Planes[3].Normal;

                    glm::vec3 nearTopLeft = plane_intersection(topPlaneNormal, 0, leftPlaneNormal,
                                                               0, nearNormal, nearD);
                    glm::vec3 nearTopRight = plane_intersection(topPlaneNormal, 0, rightPlaneNormal,
                                                                0, nearNormal, nearD);
                    glm::vec3 nearBottomLeft =
                        plane_intersection(bottomPlaneNormal, 0, leftPlaneNormal, 0, nearNormal,
                                           nearD);
                    glm::vec3 nearBottomRight =
                        plane_intersection(bottomPlaneNormal, 0, rightPlaneNormal, 0, nearNormal,
                                           nearD);

                    glm::vec3 farTopLeft =
                        plane_intersection(topPlaneNormal, 0, leftPlaneNormal, 0, farNormal, farD);
                    glm::vec3 farTopRight =
                        plane_intersection(topPlaneNormal, 0, rightPlaneNormal, 0, farNormal, farD);
                    glm::vec3 farBottomLeft =
                        plane_intersection(bottomPlaneNormal, 0, leftPlaneNormal, 0, farNormal,
                                           farD);
                    glm::vec3 farBottomRight =
                        plane_intersection(bottomPlaneNormal, 0, rightPlaneNormal, 0, farNormal,
                                           farD);

                    glm::vec3 color = glm::vec3(1, 1, 0); // yellow for example

                    // Near plane rectangle
                    nijiEngine.add_line(nearTopLeft, nearTopRight, color);
                    nijiEngine.add_line(nearTopRight, nearBottomRight, color);
                    nijiEngine.add_line(nearBottomRight, nearBottomLeft, color);
                    nijiEngine.add_line(nearBottomLeft, nearTopLeft, color);

                    // Far plane rectangle
                    nijiEngine.add_line(farTopLeft, farTopRight, color);
                    nijiEngine.add_line(farTopRight, farBottomRight, color);
                    nijiEngine.add_line(farBottomRight, farBottomLeft, color);
                    nijiEngine.add_line(farBottomLeft, farTopLeft, color);

                    // Connect near and far planes
                    nijiEngine.add_line(nearTopLeft, farTopLeft, color);
                    nijiEngine.add_line(nearTopRight, farTopRight, color);
                    nijiEngine.add_line(nearBottomLeft, farBottomLeft, color);
                    nijiEngine.add_line(nearBottomRight, farBottomRight, color);
                }
            }
        }
    }

    // Tiled Light Culling
    {
        VkBufferMemoryBarrier bufferBarrier = {};
        bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        bufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.buffer = m_dispatchParams[frameIndex].Handle;
        bufferBarrier.offset = 0;
        bufferBarrier.size = m_dispatchParams[frameIndex].Desc.Size;

        vkCmdPipelineBarrier(cmd.m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0,
                             nullptr);

        {
            nijiEngine.m_context.get_window_size(m_winWidth, m_winHeight);

            uint32_t totalThreadGroupsX = ceil((float)m_winWidth / GROUP_SIZE);
            uint32_t totalThreadGroupsY = ceil((float)m_winHeight / GROUP_SIZE);

            uint32_t totalThreadsX = totalThreadGroupsX * GROUP_SIZE;
            uint32_t totalThreadsY = totalThreadGroupsY * GROUP_SIZE;

            m_totalThreads = {totalThreadsX, totalThreadsY};
            m_totalThreadGroups = {totalThreadGroupsX, totalThreadGroupsY};

            DispatchParams ubo = {};

            auto& cameraSystem = nijiEngine.ecs.find_system<CameraSystem>();
            auto& camera = cameraSystem.m_camera;

            ubo.InverseProjection = glm::inverse(camera.GetProjectionMatrix());
            ubo.numThreadGroups = glm::u32vec3(m_totalThreadGroups, 1);
            ubo.numThreads = glm::u32vec3(m_totalThreads, 1);
            ubo.ScreenDimensions = {m_winWidth, m_winHeight};

            vkCmdUpdateBuffer(cmd.m_commandBuffer, m_dispatchParams[frameIndex].Handle, 0,
                              sizeof(DispatchParams), &ubo);
        }

        VkDebugUtilsLabelEXT labelInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
        labelInfo.pLabelName = "Tiled Light Culling Pass";
        labelInfo.color[0] = 0.2f;
        labelInfo.color[1] = 0.6f;
        labelInfo.color[2] = 0.9f;
        labelInfo.color[3] = 1.0f;

        VKCmdBeginDebugUtilsLabelEXT(cmd.m_commandBuffer, &labelInfo);

        cmd.bind_pipeline(m_lightCullingPipeline.PipelineObject, true);

        // Globals - 0
        {
            cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_COMPUTE,
                                     m_lightCullingPipeline.PipelineLayout, 0, 1,
                                     &renderer.m_globalDescriptor.m_set[renderer.m_currentFrame]);
        }

        // Per Pass - 1
        {
            m_lightCullingDescriptor.m_info.Bindings[0].Resource = &m_dispatchParams[frameIndex];

            m_lightCullingDescriptor.m_info.Bindings[1].Resource = &m_frustums[frameIndex];

            m_lightCullingDescriptor.m_info.Bindings[2].Resource = &m_lightIndexCounter[frameIndex];

            m_lightCullingDescriptor.m_info.Bindings[3].Resource =
                &renderer.m_lightIndexList[frameIndex];

            m_lightCullingDescriptor.m_info.Bindings[4].Resource = &renderer.m_lightGridTexture;

            m_depthTexture = Texture(*info.DepthAttachment);
            m_depthTexture.ImageInfo.imageLayout = info.DepthAttachment->CurrentLayout;
            m_lightCullingDescriptor.m_info.Bindings[5].Resource = &m_depthTexture;

            m_lightCullingDescriptor.m_info.Bindings[6].Resource = &renderer.m_spheres[frameIndex];
            m_lightCullingDescriptor.m_info.Bindings[7].Resource =
                &renderer.m_sceneInfoBuffer[frameIndex];

            std::vector<VkWriteDescriptorSet> writes = {};
            std::vector<VkDescriptorBufferInfo> bufferInfos = {};
            std::vector<VkDescriptorImageInfo> imageInfos = {};

            writes.reserve(m_lightCullingDescriptor.m_info.Bindings.size());
            bufferInfos.reserve(m_lightCullingDescriptor.m_info.Bindings.size());
            imageInfos.reserve(m_lightCullingDescriptor.m_info.Bindings.size());

            m_lightCullingDescriptor.push_descriptor_writes(writes, bufferInfos, imageInfos);

            cmd.push_descriptor_set(VK_PIPELINE_BIND_POINT_COMPUTE,
                                    m_lightCullingPipeline.PipelineLayout, 1,
                                    static_cast<uint32_t>(writes.size()), writes.data());
        }

        cmd.dispatch(m_totalThreadGroups.x, m_totalThreadGroups.y, 1);

        VKCmdEndDebugUtilsLabelEXT(cmd.m_commandBuffer);
    }

    

    // Syncing (for Light Grid)
    {
        VkImageMemoryBarrier2 imgBarrier = {};
        imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imgBarrier.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        imgBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        imgBarrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        imgBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        imgBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imgBarrier.image = renderer.m_lightGridTexture.TextureImage;
        imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgBarrier.subresourceRange.baseMipLevel = 0;
        imgBarrier.subresourceRange.levelCount = 1;
        imgBarrier.subresourceRange.baseArrayLayer = 0;
        imgBarrier.subresourceRange.layerCount = 1;

        VkDependencyInfo depInfo = {};
        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imgBarrier;

        VKCmdPipelineBarrier2KHR(cmd.m_commandBuffer, &depInfo);
        renderer.m_lightGridTexture.ImageInfo.imageLayout = imgBarrier.newLayout;
    }

    // Syncing (for Light Index List)
    {
        VkBufferMemoryBarrier2 bar = {};
        bar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bar.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        bar.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        bar.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        bar.buffer = renderer.m_lightIndexList[frameIndex].Handle;
        bar.size = renderer.m_lightIndexList[frameIndex].Desc.Size;

        /*VkMemoryBarrier2 memBarrier = {};
        memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        memBarrier.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memBarrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;*/

        VkDependencyInfo depInfo = {};
        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.bufferMemoryBarrierCount = 1;
        depInfo.pBufferMemoryBarriers = &bar;

        VKCmdPipelineBarrier2KHR(cmd.m_commandBuffer, &depInfo);
    }
}

void LightCullingPass::cleanup()
{
    base_cleanup();

    m_lightCullingPipeline.cleanup();
    m_lightCullingDescriptor.cleanup();

    for (int i = 0; i < m_dispatchParams.size(); i++)
    {
        m_dispatchParams[i].cleanup();
    }

    for (int i = 0; i < m_frustums.size(); i++)
    {
        m_frustums[i].cleanup();
    }

    for (int i = 0; i < m_lightIndexCounter.size(); i++)
    {
        m_lightIndexCounter[i].cleanup();
    }

    /*for (int i = 0; i < m_lightIndexList.size(); i++)
    {
        m_lightIndexList[i].cleanup();
    }*/

    // m_lightGridTexture.cleanup();
}

void LightCullingPass::on_window_resize()
{
    nijiEngine.m_context.get_window_size(m_winWidth, m_winHeight);

    uint32_t totalThreadsX = ceil((float)m_winWidth / GROUP_SIZE);
    uint32_t totalThreadsY = ceil((float)m_winHeight / GROUP_SIZE);

    uint32_t totalThreadGroupsX = ceil(float(totalThreadsX) / GROUP_SIZE);
    uint32_t totalThreadGroupsY = ceil(float(totalThreadsY) / GROUP_SIZE);

    m_totalThreads = {totalThreadsX, totalThreadsY};
    m_totalThreadGroups = {totalThreadGroupsX, totalThreadGroupsY};
}