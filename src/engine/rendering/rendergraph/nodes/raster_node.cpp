#include "raster_node.hpp"

#include <cassert>
#include <cstring>

namespace niji
{

RasterNode::RasterNode(std::string_view label, std::string_view shaderPath) : Node(label, NodeType::Raster), m_shaderPath(shaderPath)
{
}

RasterNode::~RasterNode()
{
}

RasterNode& RasterNode::write(ResourceHandle resource, DependencyStages stages)
{
    // Insert the Write Dependency
    m_dependencies.emplace_back(resource, DependencyUsage::ReadWrite, stages);
    return *this;
}

RasterNode& RasterNode::read(ResourceHandle resource, DependencyStages stages)
{
    // Insert the Read Dependency
    m_dependencies.emplace_back(resource, DependencyUsage::Readonly, stages);
    return *this;
}

RasterNode& RasterNode::push_constants(void* data, uint32_t offset, uint32_t size, DependencyStages stages)
{
    assert(offset + size <= sizeof(m_pcData) && "push constants exceed 128 bytes");
    memcpy(m_pcData + offset, data, size);
    m_rangeOffset = offset;

    const uint32_t needed = offset + size;
    if (needed > m_rangeSize)
        m_rangeSize = needed;

    m_pcStages = stages;
    return *this;
}

RasterNode& RasterNode::attach(ResourceHandle resource)
{
    m_dependencies.emplace_back(resource, DependencyUsage::ColorAttachment, DependencyStages::Pixel);
    return *this;
}

RasterNode& RasterNode::depth_stencil(TextureHandle image, bool test, bool write, StencilState stencilState)
{
    m_dependencies.emplace_back(image, DependencyUsage::DepthStencil, DependencyStages::Pixel);
    m_depthStencilImage = image;
    m_depthTest = test;
    m_depthWrite = write;
    m_stencilState = stencilState;
    return *this;
}

RasterNode& RasterNode::raster_extent(uint32_t width, uint32_t height, uint32_t x, uint32_t y)
{
    m_rasterW = width;
    m_rasterH = height;
    m_rasterX = x;
    m_rasterY = y;
    return *this;
}

DrawCall& RasterNode::draw(BufferHandle vertexBuffer, uint32_t vertexBufferPushOffset, BufferHandle indexBuffer, uint32_t indexCount, uint32_t vertexOffset, uint32_t instanceCount,
                           uint32_t instanceOffset)
{
    m_vbInjectOffset = vertexBufferPushOffset;
    return m_draws.emplace_back(*this, vertexBuffer, indexBuffer, indexCount, vertexOffset, instanceCount, instanceOffset);
}

DrawCall& RasterNode::draw_indirect(BufferHandle vertexBuffer, BufferHandle indexBuffer, BufferHandle indirectBuffer)
{
    return m_draws.emplace_back(*this, vertexBuffer, indexBuffer, indirectBuffer);
}

RasterNode& RasterNode::topology(Topology type)
{
    m_topology = type;
    return *this;
}

RasterNode& RasterNode::load_op_color(LoadOp op)
{
    m_pixelLoadOp = op;
    return *this;
}

RasterNode& RasterNode::load_op_depth(LoadOp op)
{
    m_depthLoadOp = op;
    return *this;
}

RasterNode& RasterNode::input_rate(VertexInputRate rate)
{
    m_vertexInputRate = rate;
    return *this;
}

RasterNode& RasterNode::alpha_blending(bool blend)
{
    m_alphaBlend = blend;
    return *this;
}

DrawCall::DrawCall(RasterNode& parentPass, BufferHandle vertexBuffer, BufferHandle indexBuffer, uint32_t indexCount, uint32_t vertexOffset, uint32_t instanceCount, uint32_t instanceOffset)
    : ParentPass(parentPass), VertexBuffer(vertexBuffer), IndexBuffer(indexBuffer), IndexCount(indexCount), VertexOffset(vertexOffset), InstanceCount(instanceCount),
      InstanceOffset(instanceOffset)
{
    // Vertex buffer read via BDA — dependency for barrier tracking
    if (!vertexBuffer.is_valid())
    {
        printf("Invalid Vertex Buffer Bound for Draw Call Creation \n");
        return;
    }
    parentPass.m_dependencies.emplace_back(vertexBuffer, DependencyUsage::Readonly, DependencyStages::Vertex);

    // Index buffer bound via vkCmdBindIndexBuffer — index-read dependency
    if (!indexBuffer.is_valid())
    {
        printf("Invalid Index Buffer Bound for Draw Call Creation \n");
        return;
    }
    parentPass.m_dependencies.emplace_back(indexBuffer, DependencyUsage::IndexBuffer, DependencyStages::Vertex);
}

DrawCall::DrawCall(RasterNode& parentPass, BufferHandle vertexBuffer, BufferHandle indexBuffer, BufferHandle indirectBuffer)
    : ParentPass(parentPass), VertexBuffer(vertexBuffer), IndexBuffer(indexBuffer), IndirectBuffer(indirectBuffer)
{
    if (!vertexBuffer.is_valid())
    {
        printf("Invalid Vertex Buffer Bound for Draw Call Creation \n");
        return;
    }
    parentPass.m_dependencies.emplace_back(vertexBuffer, DependencyUsage::Readonly, DependencyStages::Vertex);

    if (!indexBuffer.is_valid())
    {
        printf("Invalid Index Buffer Bound for Draw Call Creation \n");
        return;
    }
    parentPass.m_dependencies.emplace_back(indexBuffer, DependencyUsage::IndexBuffer, DependencyStages::Vertex);

    if (!indirectBuffer.is_valid())
    {
        printf("Invalid Indirect Buffer Bound for Draw Call Creation \n");
        return;
    }
    parentPass.m_dependencies.emplace_back(indirectBuffer, DependencyUsage::IndirectBuffer, DependencyStages::Vertex);
}

} // namespace niji