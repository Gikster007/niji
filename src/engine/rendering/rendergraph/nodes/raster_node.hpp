#pragma once

#include <string_view>
#include <vector>

#include "node.hpp"

namespace niji
{

struct DrawCall;

// Vertex Primitive Topology
enum class Topology : uint32_t
{
    Invalid = 0u, // Invalid primitive topology
    TriangleList, // List of triangles
    LineList,     // List of lines
};

enum class CullMode : uint32_t
{
    None,
    Front,       // Cull Front Face Triangles
    Back,        // Cull Back Face Triangles
    FrontAndBack // Cull Front and Back Face Triangles
};

// Pixel Load Operation
enum class LoadOp : uint32_t
{
    Load,  // Load pixel data already present
    Clear, // Clear pixel data
};

// Vertex Input Rate
enum class VertexInputRate : uint32_t
{
    Vertex,   // Advance one vertex with each vertex
    Instance, // Advance one vertex with each instance
};

// Stencil Test Operation
enum class StencilOp : uint32_t
{
    Keep,           // Keep existing stencil value
    Zero,           // Set stencil value to zero
    Replace,        // Replace stencil value with the reference value
    IncrementClamp, // Increment stencil value and clamp to 255
    DecrementClamp, // Decrement stencil value and clamp to 0
    Invert,         // Invert each bit of the stencil value
    IncrementWrap,  // Increment stencil value and wrap to 0 (256 -> 0)
    DecrementWrap,  // Decrement stencil value and wrap to 255 (-1 -> 255)
};

// Stencil Comparison Operation
enum class CompareOp : uint32_t
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

// Stencil State
struct StencilState
{
    StencilState() = default;

    StencilOp FailOp = StencilOp::Replace;
    StencilOp PassOp = StencilOp::Replace;
    StencilOp DepthFailOp = StencilOp::Replace;

    CompareOp Compare = CompareOp::Always;
    uint32_t CompareMask = 0x00u; // Comparison mask
    uint32_t WriteMask = 0x00u;   // Write mask
    uint32_t ReferenceValue = 0u; // Value to write/compare against

    bool Test = false;           // Perform stencil test
    LoadOp Load = LoadOp::Clear; // Operation to perform on load in a raster pass (load or clear)
};

// Render Graph Rasterization Node
class RasterNode : public Node
{
  public:
    RasterNode(std::string_view label, std::string_view shaderPath);
    ~RasterNode();

    // No Copy
    RasterNode(const RasterNode&) = delete;
    RasterNode& operator=(const RasterNode&) = delete;

    // Set the Vertex Primitive Topology of the Pass
    RasterNode& topology(Topology type);

    // Set the Vertex Cull Mode of the Pass
    RasterNode& cull_mode(CullMode mode);

    // Set the Pixel Load Operation of the Pass
    RasterNode& load_op_color(LoadOp op);

    // Set the Depth Load Operation of the Pass
    RasterNode& load_op_depth(LoadOp op);

    // Set the Vertex Input Rate of the Pass
    RasterNode& input_rate(VertexInputRate rate);

    // Set the Alpha Blending of the Pass
    RasterNode& alpha_blending(bool blend);

    // Add a Bindable Resource as an Output for this Node
    RasterNode& write(ResourceHandle resource, DependencyStages stages);

    // Add a Bindable Resource as an Input for this Node
    RasterNode& read(ResourceHandle resource, DependencyStages stages);

    // Set Push Constants for this Node
    // offset: Offset of the push constants in bytes
    // size: Size of the push constants in bytes
    RasterNode& push_constants(void* data, uint32_t offset, uint32_t size, DependencyStages stages);

    // Add a Rendering Attachment as an Output for the Pixel Stage
    RasterNode& attach(ResourceHandle resource);

    // Add a Depth/Stencil Attachment as an Input/Output
    RasterNode& depth_stencil(TextureHandle image, bool test = true, bool write = true, StencilState stencilState = StencilState());

    // Set the Raster Extent of the Raster Pass (the extent of the attachments to rasterize into)
    RasterNode& raster_extent(uint32_t width, uint32_t height, uint32_t x = 0u, uint32_t y = 0u);

    // Create a Draw Call for this Raster Pass
    DrawCall& draw(BufferHandle vertexBuffer, uint32_t vertexBufferPushOffset, BufferHandle indexBuffer, uint32_t indexCount, uint32_t vertexOffset = 0u, uint32_t instanceCount = 1u,
                   uint32_t instanceOffset = 0u);

    // Create an Indirect Draw Call for this Raster Pass
    DrawCall& draw_indirect(BufferHandle vertexBuffer, BufferHandle indexBuffer, BufferHandle indirectBuffer);

  public:
    // Shader File Path
    std::string_view m_shaderPath {};

    Topology m_topology = Topology::Invalid;
    CullMode m_cullMode = CullMode::None;
    LoadOp m_pixelLoadOp = LoadOp::Load;
    VertexInputRate m_vertexInputRate = VertexInputRate::Vertex;
    bool m_alphaBlend = false;

    // Depth/Stencil Image
    TextureHandle m_depthStencilImage {};
    LoadOp m_depthLoadOp = LoadOp::Load;
    bool m_depthTest = true;
    bool m_depthWrite = true;
    StencilState m_stencilState {};

    // Push Constant Stages
    DependencyStages m_pcStages {};
    // Vertex Buffer BDA Push Constants Offset
    uint32_t m_vbInjectOffset = UINT32_MAX;

    // Extents
    uint32_t m_rasterW = 0u;
    uint32_t m_rasterH = 0u;
    uint32_t m_rasterX = 0u;
    uint32_t m_rasterY = 0u;

    // Draw Calls
    std::vector<DrawCall> m_draws {};
};

struct DrawCall
{
    RasterNode& ParentPass;

    BufferHandle VertexBuffer {};   // BDA — resolved to address
    BufferHandle IndexBuffer {};    // bound via vkCmdBindIndexBuffer
    BufferHandle IndirectBuffer {}; // for draw_indirect

    uint32_t IndexCount = 0u;
    uint32_t VertexOffset = 0u;
    uint32_t InstanceCount = 0u;
    uint32_t InstanceOffset = 0u;

    // Indexed Draw Call Constructor
    DrawCall(RasterNode& parentPass, BufferHandle vertexBuffer, BufferHandle indexBuffer, uint32_t indexCount, uint32_t vertexOffset, uint32_t instanceCount, uint32_t instanceOffset);

    // Indirect Draw Call Constructor
    DrawCall(RasterNode& parentPass, BufferHandle vertexBuffer, BufferHandle indexBuffer, BufferHandle indirectBuffer);
};

} // namespace niji