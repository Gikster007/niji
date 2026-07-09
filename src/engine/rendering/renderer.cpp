#include "renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>


#include <imgui.h>
#include <stb_image.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

using namespace niji;

#include "../../app/camera_system.hpp"

#include "core/components/render-components.hpp"
#include "core/components/transform.hpp"
#include "core/logger.hpp"
#include "core/editor/editor.hpp"

#include "resource_bank.hpp"
#include "rendergraph/rendergraph.hpp"
#include "rendergraph/nodes/compute_node.hpp"
#include "rendergraph/nodes/raster_node.hpp"

#include "model/model.hpp"

#include "engine.hpp"

Renderer::Renderer() : m_resourceBank(*new ResourceBank()), m_renderGraph(*new RenderGraph())
{
    m_context = &nijiEngine.m_context;
}

Renderer::~Renderer()
{
    delete &m_resourceBank;
    delete &m_renderGraph;
}

inline static void CreateCube(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices)
{
    vertices = {
        {-1.0f, 1.0f, -1.0f},  // 0
        {-1.0f, -1.0f, -1.0f}, // 1
        {1.0f, -1.0f, -1.0f},  // 2
        {1.0f, 1.0f, -1.0f},   // 3
        {-1.0f, 1.0f, 1.0f},   // 4
        {-1.0f, -1.0f, 1.0f},  // 5
        {1.0f, -1.0f, 1.0f},   // 6
        {1.0f, 1.0f, 1.0f}     // 7
    };

    indices = {
        0, 1, 2, 2, 3, 0, // Front face
        4, 5, 1, 1, 0, 4, // Left face
        7, 6, 5, 5, 4, 7, // Back face
        3, 2, 6, 6, 7, 3, // Right face
        4, 0, 3, 3, 7, 4, // Top face
        1, 5, 6, 6, 2, 1  // Bottom face
    };
}

void Renderer::init()
{
    // Init Resource Bank
    m_resourceBank.set_max_textures(4);
    m_resourceBank.set_max_buffers(6);
    m_resourceBank.set_max_samplers(4);
    m_resourceBank.init();

    // Global Sampler
    {
        SamplerDesc desc = {};
        desc.MagFilter = SamplerDesc::Filter::NEAREST;
        desc.MinFilter = SamplerDesc::Filter::NEAREST;
        desc.AddressModeU = SamplerDesc::AddressMode::EDGE_CLAMP;
        desc.AddressModeV = SamplerDesc::AddressMode::EDGE_CLAMP;
        desc.AddressModeW = SamplerDesc::AddressMode::EDGE_CLAMP;
        desc.EnableAnisotropy = true;
        desc.MipmapMode = SamplerDesc::MipMapMode::NEAREST;
        desc.Name = "Global Sampler";

        m_globalSampler = nijiEngine.m_renderer.m_resourceBank.create_sampler(desc);
    }

    // Camera Data
    {
        BufferDesc bufferDesc = {};
        bufferDesc.Name = "Camera UBO";
        bufferDesc.Size = sizeof(CameraData);
        bufferDesc.Usage = BufferUsage::Uniform | BufferUsage::TransferDst;
        m_cameraData = m_resourceBank.create_buffer(bufferDesc);
    }

    // Point Lights (Sphere Data Only)
    {
        // Create Point Light Buffer
        {
            BufferDesc bufferDesc = {};
            bufferDesc.Name = "Point Lights (Sphere) Data";
            bufferDesc.Size = sizeof(Sphere) * MAX_POINT_LIGHTS;
            bufferDesc.Usage = BufferUsage::Storage | BufferUsage::TransferDst; // TODO: Why is it a Storage buffer?
            m_spheres = m_resourceBank.create_buffer(bufferDesc);
        }
    }

    // Create Scene Info Buffer
    {
        BufferDesc bufferDesc = {};
        bufferDesc.Name = "Scene Info Data";
        bufferDesc.Size = sizeof(SceneInfo);
        bufferDesc.Usage = BufferUsage::Uniform | BufferUsage::TransferDst;
        m_sceneInfoBuffer = m_resourceBank.create_buffer(bufferDesc);
    }

    // Init Render Target
    {
        int w, h;
        nijiEngine.m_context.get_window_size(w, h);
        m_renderInfo.RenderArea = {{0, 0}, {(uint32_t)w, (uint32_t)h}};
        m_renderInfo.RenderTarget = m_resourceBank.create_render_target((uint32_t)w, (uint32_t)h);

        m_renderGraph.set_render_target(m_renderInfo.RenderTarget);
        m_renderGraph.init();
    }

    // Init Render Info
    {
        int w, h;
        nijiEngine.m_context.get_window_size(w, h);

        TextureDesc viewportDesc {};
        viewportDesc.Name = "Viewport Texture";
        viewportDesc.Format = TextureFormat::RGBA8Unorm;
        viewportDesc.Size = {(uint32_t)w, (uint32_t)h, 0u};
        viewportDesc.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled | TextureUsage::Storage;
        viewportDesc.ShowInImGui = true;
        m_renderInfo.ViewportTexture = m_resourceBank.create_texture(viewportDesc);

        TextureDesc depthDesc {};
        depthDesc.Name = "Depth Texture";
        depthDesc.Format = TextureFormat::D32SFloat;
        depthDesc.Size = {(uint32_t)w, (uint32_t)h, 0u};
        depthDesc.Usage = TextureUsage::DepthStencil;
        m_renderInfo.DepthTexture = m_resourceBank.create_texture(depthDesc);
    }

    // Fallback Texture
    {
        int width = -1, height = -1, channels = -1;
        unsigned char* imageData = nullptr;

        imageData = stbi_load("assets/missing.png", &width, &height, &channels, STBI_rgb_alpha);

        if (!imageData)
        {
            printf("[Renderer] Failed to load Fallback Texture");
        }

        TextureDesc texDesc = {};
        texDesc.Size = {(uint32_t)width, (uint32_t)height, 0u};
        texDesc.Format = TextureFormat::RGBA8Unorm;
        texDesc.Usage = TextureUsage::TransferDst | TextureUsage::Sampled;
        texDesc.Name = "Fallback Texture";

        m_fallbackTexture = m_resourceBank.create_texture(texDesc);
        m_resourceBank.upload_texture(m_fallbackTexture, imageData, width * height * 4);

        stbi_image_free(imageData);
    }

    // Create Cube
    {
        std::vector<glm::vec3> vertices = {};
        std::vector<uint32_t> indices = {};
        CreateCube(vertices, indices);

        m_cube = Mesh(vertices, indices);
    }

    // int width = 0, height = 0;
    // nijiEngine.m_context.get_window_size(width, height);

    // uint32_t totalThreadsX = ceil((float)width / GROUP_SIZE);
    // uint32_t totalThreadsY = ceil((float)height / GROUP_SIZE);
    // glm::u32vec2 totalThreads = {totalThreadsX, totalThreadsY};
    //// Create Light Grid RWTexture
    //{

    //    TextureDesc desc = {};
    //    desc.Width = totalThreads.x;
    //    desc.Height = totalThreads.y;
    //    desc.Channels = 2;
    //    desc.IsMipMapped = false;
    //    desc.Data = nullptr;
    //    desc.Format = VK_FORMAT_R32G32_UINT;
    //    desc.MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    //    desc.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    //    desc.IsReadWrite = true;
    //    desc.ShowInImGui = true;
    //    m_lightGridTexture = Texture(desc);
    //}

    //// Create Light Index List Buffer
    //{
    //    VkDeviceSize bufferSize = sizeof(LightIndexList);
    //    m_lightIndexList.resize(MAX_FRAMES_IN_FLIGHT);
    //    const uint32_t totalTiles = totalThreads.x * totalThreads.y;
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

    nijiEngine.m_logger.log_info("Info Test");
    nijiEngine.m_logger.log_warning("Warning Test");
    nijiEngine.m_logger.log_error("Error Test");
    nijiEngine.m_logger.log_error("Error Test");
    nijiEngine.m_logger.log_error("Error Test");
    nijiEngine.m_logger.log_error("Error Test");
}

void Renderer::update(const float dt)
{
    // ImGui_ImplVulkan_NewFrame();
    // ImGui_ImplGlfw_NewFrame();
    // ImGui::NewFrame();

    // VkFence frameFence = m_inFlightFences[m_currentFrame];
    // vkWaitForFences(m_context->m_device, 1, &frameFence, VK_TRUE, UINT64_MAX);
    // vkResetFences(m_context->m_device, 1, &frameFence);

    // auto& cmd = m_commandBuffers[m_currentFrame];
    // cmd.begin_list("Frame Commmand Buffer");

    // for (auto& pass : m_renderPasses)
    //{
    //     pass->update(*this, cmd);
    // }
}

void Renderer::render()
{
    // Begin New Frame
    m_renderGraph.new_frame();

    // clang-format off
    // Record Passes

    //struct Constants 
    //{
    //    uint32_t tex {};
    //    uint32_t viewport {};
    //    uint32_t rt {};
    //} pc {};
    //
    //pc.tex = m_fallbackTexture.Index;
    //pc.viewport = m_renderInfo.ViewportTexture.Index;
    //pc.rt = 0u;

    static bool open = true;
    ImGui::ShowMetricsWindow(&open);

    //m_renderGraph.add_compute_node("fox shader", "fox_cs")
    //             .read(m_fallbackTexture)
    //             .write(m_renderInfo.ViewportTexture)
    //             .push_constants(&pc, 0u, sizeof(Constants))
    //             .group_size(8u, 8u, 1u)
    //             .work_size(1920u, 1080u, 1u);
    //

    struct RasterPush
    {
        uint64_t   vertices;   // 8 bytes — graph injects the BDA here
    } pc {};

    m_renderGraph.add_raster_node("raster pass", "test")
                 .input_rate(VertexInputRate::Vertex)
                 .load_op_color(LoadOp::Clear)
                 .load_op_depth(LoadOp::Clear)
                 .topology(Topology::TriangleList)
                 .depth_stencil(m_renderInfo.DepthTexture)
                 .attach(m_renderInfo.ViewportTexture)
                 .raster_extent(1920u, 1080u)
                 .push_constants(&pc, 0u, sizeof(RasterPush), DependencyStages::Vertex)
                 .draw(m_cube.m_vertexBuffer, 0u, m_cube.m_indexBuffer, m_cube.m_indexCount);

    //m_renderGraph.add_compute_node("cow shader", "cow_cs")
    //             .read(m_renderInfo.ViewportTexture)
    //             .write(m_renderInfo.RenderTarget, offsetof(Constants, rt))
    //             .push_constants(&pc, 0u, sizeof(Constants))
    //             .group_size(8u, 8u, 1u)
    //             .work_size(1920u, 1080u, 1u);
    // clang-format on

    nijiEngine.m_editor.render();

    // Topological Sort
    // ...

    // Execute Render Graph
    m_renderGraph.execute();
}

void Renderer::deinit()
{
    m_resourceBank.destroy(m_renderInfo.RenderTarget);
    m_resourceBank.destroy(m_renderInfo.ViewportTexture);
    m_resourceBank.destroy(m_renderInfo.DepthTexture);

    m_resourceBank.destroy(m_fallbackTexture);
    m_resourceBank.destroy(m_cameraData);
    m_resourceBank.destroy(m_globalSampler);
    m_resourceBank.destroy(m_spheres);
    m_resourceBank.destroy(m_sceneInfoBuffer);

    m_cube.deinit();

    m_resourceBank.deinit();
    m_renderGraph.deinit();
}

void Renderer::update_uniform_buffer()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    auto& cameraSystem = nijiEngine.ecs.find_system<CameraSystem>();
    auto& camera = cameraSystem.m_camera;
    {
        CameraData ubo = {};

        ubo.View = camera.GetViewMatrix();
        ubo.Proj = camera.GetProjectionMatrix();
        ubo.Pos = camera.Position;

        ubo.Proj[1][1] *= -1;
        m_resourceBank.upload_buffer(m_cameraData, &ubo, 0u, sizeof(CameraData));
    }

    {
        std::vector<Sphere> pointLightsArray = {};
        {
            auto pointLightView = nijiEngine.ecs.m_registry.view<PointLight>();
            for (const auto& [ent, pointLight] : pointLightView.each())
            {
                if (pointLightsArray.size() < MAX_POINT_LIGHTS)
                {
                    Sphere s = {};
                    glm::vec4 centerWS = glm::vec4(pointLight.Position, 1.0f);
                    s.Center = glm::vec3(camera.GetViewMatrix() * centerWS);
                    // printf("World Space Pos: %f, %f, %f \n", centerWS.x, centerWS.y, centerWS.z);
                    // printf("View Space Pos: %f, %f, %f \n", s.Center.x, s.Center.y, s.Center.z);
                    s.Radius = pointLight.Range;
                    pointLightsArray.push_back(s);
                }
            }
            m_resourceBank.upload_buffer(m_spheres, pointLightsArray.data(), 0u, sizeof(Sphere) * pointLightsArray.size());
        }

        auto dirLightView = nijiEngine.ecs.m_registry.view<DirectionalLight>();
        for (const auto& [ent, dirLight] : dirLightView.each())
        {
            SceneInfo sceneInfo = {};
            sceneInfo.DirLight = dirLight;
            sceneInfo.PointLightCount = pointLightsArray.size();

            m_resourceBank.upload_buffer(m_sceneInfoBuffer, &sceneInfo, 0u, sizeof(SceneInfo));
        }
    }
}