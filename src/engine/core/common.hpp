#pragma once

#include <string>

#include <glm/glm.hpp>

enum VmaMemoryUsage;
struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

namespace niji
{
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

struct BufferDesc
{
    VkDeviceSize Size = {};

    enum class BufferUsage
    {
        Invalid,
        Vertex,
        Index,
        Uniform,
        Storage
    } Usage = {};

    bool IsPersistent = false;
    char* Name = "Unknown Buffer";
};

struct Buffer
{
    Buffer() = default;
    Buffer(BufferDesc& desc, void* data);
    ~Buffer();

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    void cleanup();

    VkBuffer Handle = {};
    VmaAllocation BufferAllocation = {};
    BufferDesc Desc = {};

    void* Data = nullptr;
    bool Mapped = false;
};

// TODO: Refactor (create RenderTargetInfo struct which holds construct info that will be passed in
// the constructor of RenderTarget)
struct RenderTarget
{
    RenderTarget() = default;
    RenderTarget(VkImage image, VkImageView view, VkFormat Format = VK_FORMAT_UNDEFINED,
                 VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED,
                 VkAttachmentLoadOp load = VK_ATTACHMENT_LOAD_OP_CLEAR,
                 VkAttachmentStoreOp store = VK_ATTACHMENT_STORE_OP_STORE, VkClearValue clear = {});

    VkImage Image = VK_NULL_HANDLE;
    VkImageView ImageView = VK_NULL_HANDLE;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    VkImageLayout CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkClearValue ClearValue = {};

    char* Name = nullptr;
};

struct RenderInfo
{
    RenderInfo() = default;
    RenderInfo(const VkExtent2D& extent)
    {
        RenderArea.offset = {0, 0};
        RenderArea.extent = extent;
    }

    VkRect2D RenderArea = {{0, 0}, {0, 0}};
    uint32_t LayerCount = 1;

    RenderTarget* ColorAttachment = nullptr;
    RenderTarget* DepthAttachment = nullptr;

    bool HasDepth = false;
    bool PrepareForPresent = false;
};

struct TextureDesc
{
    int Width = 0;
    int Height = 0;
    int Channels = 4;
    unsigned char* Data = nullptr;
    char* Name = nullptr;

    enum class TextureType
    {
        NONE,
        TEXTURE_2D,
        CUBEMAP
    } Type = TextureType::NONE;

    bool IsReadWrite = false;
    bool IsMipMapped = false;
    bool ShowInImGui = false;
    uint32_t Mips = 1;
    uint32_t Layers = 1;

    VkFormat Format = {};
    VkImageUsageFlags Usage = {};
    VmaMemoryUsage MemoryUsage = {};
};

struct Texture
{
    Texture() = default;
    Texture(const TextureDesc& desc);
    // Used only for Envmap Loading
    Texture(const TextureDesc& desc, const std::string& path);
    // Used only for translating a Depth RT
    Texture(RenderTarget& depthRT)
    {
        TextureImage = depthRT.Image;
        TextureImageView = depthRT.ImageView;
        Desc.Format = depthRT.Format;
        Desc.Type = TextureDesc::TextureType::TEXTURE_2D;

        // Populate descriptor info so it can be used in descriptor sets
        ImageInfo.imageLayout = depthRT.ImageLayout;
        ImageInfo.imageView = depthRT.ImageView;
        ImageInfo.sampler = VK_NULL_HANDLE; // You can assign a sampler if needed
    }

    void cleanup() const;

    TextureDesc Desc = {};

    VkImage TextureImage = {};
    VkImageView TextureImageView = {};
    VmaAllocation TextureImageAllocation = {};

    VkDescriptorImageInfo ImageInfo = {};
    
    VkDescriptorSet ImGuiHandle = {};
};

struct SamplerDesc
{
    enum class Filter
    {
        NONE,
        NEAREST,
        LINEAR
    } MagFilter = Filter::NONE;
    Filter MinFilter = Filter::NONE;

    enum class AddressMode
    {
        NONE,
        REPEAT,
        MIRRORED_REPEAT,
        EDGE_CLAMP,
        BORDER_CLAMP,
        MIRRORED_EDGE_CLAMP
    } AddressModeU = AddressMode::NONE;
    AddressMode AddressModeV = AddressMode::NONE;
    AddressMode AddressModeW = AddressMode::NONE;

    enum class MipMapMode
    {
        NONE,
        NEAREST,
        LINEAR
    } MipmapMode = MipMapMode::NONE;

    bool EnableAnisotropy = false;
    uint32_t MaxMips = 0;

    char* Name = {};
};

struct Sampler
{
    Sampler() = default;
    Sampler(const SamplerDesc& desc);

    void cleanup() const;

    SamplerDesc Desc = {};

    VkSampler Handle = {};
};

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
    VkDescriptorSetLayout GlobalDescriptorSetLayout;
    VkDescriptorSetLayout PassDescriptorSetLayout;
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
    VkDescriptorSetLayout GlobalDescriptorSetLayout;
    VkDescriptorSetLayout PassDescriptorSetLayout;
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

    GraphicsPipelineDesc GraphicsDesc;
    ComputePipelineDesc ComputeDesc;

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
