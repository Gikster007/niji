#include "line_render_pass.hpp"

#include "rendering/renderer.hpp"
#include "engine.hpp"

using namespace niji;

void LineRenderPass::init(Swapchain& swapchain, Descriptor& globalDescriptor)
{
    m_name = "Line Render Pass";

    // Init Vertex Buffer
    {
        BufferDesc desc = {};
        desc.IsPersistent = true;
        desc.Size = sizeof(DebugLine) * MAX_DEBUG_LINES;
        desc.Usage = BufferDesc::BufferUsage::Vertex;
        desc.Name = "Debug Lines Vertex Buffer";
        m_vertexBuffer = Buffer(desc, nijiEngine.m_debugLines.data());
    }

    GraphicsPipelineDesc pipelineDesc = {globalDescriptor.m_setLayout,
                                         m_passDescriptor.m_setLayout};

    add_shader("shaders/line_render_pass.slang", ShaderType::FRAG_AND_VERT);
    pipelineDesc.Name = "Line Render Pass";
    pipelineDesc.VertexShader = m_vertFrag.Spirv[0];
    pipelineDesc.FragmentShader = m_vertFrag.Spirv[1];

    pipelineDesc.Rasterizer.CullMode = RasterizerState::CullingMode::BACK;
    pipelineDesc.Rasterizer.PolyMode = RasterizerState::PolygonMode::LINE;
    pipelineDesc.Rasterizer.RasterizerDiscardEnable = false;
    pipelineDesc.Rasterizer.DepthClampEnable = false;
    pipelineDesc.Rasterizer.LineWidth = 1.0f;

    pipelineDesc.DepthTestEnable = true;
    pipelineDesc.DepthWriteEnable = false;
    pipelineDesc.DepthCompareOperation = GraphicsPipelineDesc::DepthCompareOp::LESS;

    pipelineDesc.Topology = GraphicsPipelineDesc::PrimitiveTopology::LINE_LIST;

    pipelineDesc.Viewport.Width = swapchain.m_extent.width;
    pipelineDesc.Viewport.Height = swapchain.m_extent.height;
    pipelineDesc.Viewport.MaxDepth = 1.0f;
    pipelineDesc.Viewport.MinDepth = 0.0f;
    pipelineDesc.Viewport.ScissorWidth = swapchain.m_extent.width;
    pipelineDesc.Viewport.ScissorHeight = swapchain.m_extent.height;

    pipelineDesc.ColorAttachmentFormat = swapchain.m_format;

    pipelineDesc.VertexLayout =
        DEFINE_VERTEX_LAYOUT(DebugLine,
                             VertexElement(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(DebugLine, Pos)),
                             VertexElement(1, VK_FORMAT_R32G32B32_SFLOAT,
                                           offsetof(DebugLine, Color)));

    m_pipelines.emplace(pipelineDesc.Name, Pipeline(pipelineDesc));
}

void LineRenderPass::update_impl(Renderer& renderer, CommandList& cmd)
{
    if (nijiEngine.m_debugLines.size() > 0)
    {
        /*vkCmdUpdateBuffer(cmd.m_commandBuffer, m_vertexBuffer.Handle, 0,
                          sizeof(DebugLine) * nijiEngine.m_debugLines.size(),
                          nijiEngine.m_debugLines.data());*/
        memcpy(m_vertexBuffer.Data, nijiEngine.m_debugLines.data(),
               sizeof(DebugLine) * nijiEngine.m_debugLines.size());
    }

    nijiEngine.m_debugLines.clear();
}

void LineRenderPass::record(Renderer& renderer, CommandList& cmd, RenderInfo& info)
{
    Swapchain& swapchain = renderer.m_swapchain;
    const uint32_t& frameIndex = renderer.m_currentFrame;
    const Pipeline& pipeline = m_pipelines.at("Line Render Pass");

    info.ViewportTarget->StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    info.ViewportTarget->LoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

    info.DepthAttachment->StoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    info.DepthAttachment->LoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

    cmd.begin_rendering(info, m_name);

    cmd.bind_pipeline(pipeline.PipelineObject);

    cmd.bind_viewport(swapchain.m_extent);
    cmd.bind_scissor(swapchain.m_extent);

    VkBuffer vertexBuffers[] = {m_vertexBuffer.Handle};
    VkDeviceSize offsets[] = {0};
    cmd.bind_vertex_buffer(0, 1, vertexBuffers, offsets);

    // Globals - 0
    {
        cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.PipelineLayout, 0, 1,
                                 &renderer.m_globalDescriptor.m_set[renderer.m_currentFrame]);
    }

    cmd.draw(nijiEngine.m_debugLines.size(), 1, 0, 0);

    cmd.end_rendering(info);
}

void LineRenderPass::cleanup()
{
    base_cleanup();

    m_vertexBuffer.cleanup();
}