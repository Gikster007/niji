#pragma once

#include <string>

#include <glm/glm.hpp>

enum VmaMemoryUsage;
struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

namespace niji
{
// TODO: Move this to context class
template <typename HandleType>
void SetObjectName(VkDevice device, VkObjectType type, HandleType handle, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = type;
    nameInfo.objectHandle = reinterpret_cast<uint64_t>(handle);
    nameInfo.pObjectName = name;

    auto func =
        (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device,
                                                              "vkSetDebugUtilsObjectNameEXT");
    if (func)
    {
        func(device, &nameInfo);
    }
}

uint32_t GetBytesPerTexel(VkFormat format);

// Source BEE engine
std::vector<char> read_binary_file(const std::string& path);

std::vector<char> read_file(const std::string& filename);

struct Vertex
{
    glm::vec3 Pos = {};
    glm::vec3 Color = {};
    glm::vec3 Normal = {};
    glm::vec4 Tangent = {};
    glm::vec2 TexCoord = {};
};

struct SkyboxVertex
{
    glm::vec3 Pos = {};
};

struct DebugLine
{
    glm::vec3 Pos = {};
    glm::vec3 Color = {};
};

struct MaterialInfo
{
    alignas(16) glm::vec4 AlbedoFactor = glm::vec4(1.0f);
    alignas(16) glm::vec4 EmissiveFactor = glm::vec4(0.0f);
    alignas(4) float MetallicFactor = 1.0f;
    alignas(4) float RoughnessFactor = 1.0f;
    alignas(4) int HasNormalMap = 0;
    alignas(4) int HasEmissiveMap = 0;
    alignas(4) int HasMetallicMap = 0;
    alignas(4) int HasRoughnessMap = 0;

    alignas(16) int _padding[4] = {};
};

struct CameraData
{
    alignas(16) glm::mat4 View = {};
    alignas(16) glm::mat4 Proj = {};
    alignas(16) glm::vec3 Pos = {};
};
struct ModelData
{
    alignas(16) glm::mat4 Model = {};
    alignas(16) glm::mat4 InvModel = {};

    alignas(16) MaterialInfo MaterialInfo = {};
};
struct DebugSettings
{
    enum class RenderFlags
    {
        ALBEDO,
        UVS,
        GEO_NORMAL,
        SHADING_NORMAL,
        NORMAL_MAP,
        TANGENT,
        BITANGENT,
        OCCLUSION,
        EMISSIVE,
        METALLIC,
        ROUGHNESS,
        NONE,
        COUNT
    } RenderMode = RenderFlags::NONE;

    bool DrawLightHeatmap = false;
    bool _pad0;
    uint16_t _pad1;
};
static const char* RenderFlagNames[] = {"Albedo",         "UVs",        "Geometry Normal",
                                        "Shading Normal", "Normal Map", "Tangent",
                                        "Bitangent",      "Occlusion",  "Emissive",
                                        "Metallic",       "Roughness",  "None"};

struct VertexLayout
{
    VertexLayout() = default;

    VkVertexInputBindingDescription Binding = {};
    std::vector<VkVertexInputAttributeDescription> Attributes = {};
};

struct VertexElement
{
    VertexElement() = default;

    uint32_t Location = {};
    VkFormat Format = {};
    size_t Offset = {};
};

#define DEFINE_VERTEX_LAYOUT(type, ...)                                                            \
    []() -> VertexLayout {                                                                         \
        VertexLayout layout = {};                                                                  \
        layout.Binding.binding = 0;                                                                \
        layout.Binding.stride = sizeof(type);                                                      \
        layout.Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;                                    \
        layout.Attributes = {__VA_ARGS__};                                                         \
        return layout;                                                                             \
    }()

#define VertexElement(Location, Format, Offset)                                                    \
    VkVertexInputAttributeDescription                                                              \
    {                                                                                              \
        Location, 0, Format, Offset                                                                \
    }

struct Viewport
{
    Viewport() = default;

    // Viewport Related
    float Width = 0.0f;
    float Height = 0.0f;
    float MinDepth = 0.0f;
    float MaxDepth = 1.0f;
    // Scissor Related
    uint32_t ScissorWidth = 0;
    uint32_t ScissorHeight = 0;
};

struct RasterizerState
{
    RasterizerState() = default;

    bool DepthClampEnable = false;
    bool RasterizerDiscardEnable = false;
    enum class PolygonMode
    {
        FILL,
        LINE,
        POINT
    } PolyMode = PolygonMode::FILL;
    enum class CullingMode
    {
        NONE,
        FRONT,
        BACK,
        FRONT_AND_BACK
    } CullMode = CullingMode::NONE;
    float LineWidth = 1.0f;
};

struct GraphicsPipelineDesc
{
    GraphicsPipelineDesc() = default;
    GraphicsPipelineDesc(VkDescriptorSetLayout& globalLayout, VkDescriptorSetLayout& passlLayout)
        : GlobalDescriptorSetLayout(globalLayout), PassDescriptorSetLayout(passlLayout)
    {
    }

    std::string VertexShader = {};
    std::string FragmentShader = {};
    VertexLayout VertexLayout = {};

    enum class PrimitiveTopology
    {
        POINT,
        LINE_LIST,
        LINE_STRIP,
        TRIANGLE_LIST,
        TRIANGLE_STRIP
    } Topology = PrimitiveTopology::TRIANGLE_LIST;
    enum class DepthCompareOp
    {
        NEVER,
        LESS,
        EQUAL,
        LESS_OR_EQUAL,
        GREATER,
        NOT_EQUAL,
        GREATER_OR_EQUAL,
        ALWAYS
    } DepthCompareOperation = DepthCompareOp::LESS;
    bool DepthTestEnable = true;
    bool DepthWriteEnable = true;

    int ColorAttachmentCount = 1;

    Viewport Viewport = {};
    RasterizerState Rasterizer = {};
    VkFormat ColorAttachmentFormat = {};

    char* Name = "Unknown Graphics Pipeline";

  private:
    friend class Pipeline;
    VkDescriptorSetLayout GlobalDescriptorSetLayout = {};
    VkDescriptorSetLayout PassDescriptorSetLayout = {};
};

struct ComputePipelineDesc
{
    ComputePipelineDesc() = default;
    ComputePipelineDesc(VkDescriptorSetLayout& globalLayout, VkDescriptorSetLayout& passlLayout)
        : GlobalDescriptorSetLayout(globalLayout), PassDescriptorSetLayout(passlLayout)
    {
    }

    std::string ComputeShader = {};
    char* Name = "Unknown Compute Pipeline";

  private:
    friend class Pipeline;
    VkDescriptorSetLayout GlobalDescriptorSetLayout = {};
    VkDescriptorSetLayout PassDescriptorSetLayout = {};
};

struct Pipeline
{
    Pipeline() = default;
    Pipeline(GraphicsPipelineDesc desc);
    Pipeline(ComputePipelineDesc desc);

    void cleanup();

    VkPipeline PipelineObject = {};
    VkPipelineLayout PipelineLayout = {};
    char* Name = nullptr;

    GraphicsPipelineDesc GraphicsDesc = {};
    ComputePipelineDesc ComputeDesc = {};

    bool IsGraphicsPipeline = true;
};

// TODO: Switch naming style to convention (All caps with "_" between words)
enum class TransitionType
{
    Undefined,
    ColorAttachment,
    DepthStencilAttachmentWrite,
    DepthStencilAttachmentLate,
    Present,
    ShaderRead,  // sampled image
    TransferDst, // for upload
    TransferSrc  // for copy
};

struct TransitionInfo
{
    VkPipelineStageFlags Stage = {};
    VkAccessFlags Access = {};
    VkImageLayout Layout = {};
};

TransitionInfo usage_to_barrier(TransitionType usage, VkFormat format);

} // namespace niji
