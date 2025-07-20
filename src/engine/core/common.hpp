#pragma once

#include <string>

#include <glm/glm.hpp>

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

struct Vertex
{
    glm::vec3 Pos = {};
    glm::vec3 Color = {};
    glm::vec3 Normal = {};
    glm::vec4 Tangent = {};
    glm::vec2 TexCoord = {};
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

struct NijiTexture
{
    void cleanup() const;

    VkImage TextureImage = {};
    VkImageView TextureImageView = {};
    VmaAllocation TextureImageAllocation = {};

    VkDescriptorImageInfo ImageInfo = {};
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

struct PipelineDesc
{
    PipelineDesc(VkDescriptorSetLayout& globalLayout, VkDescriptorSetLayout& passlLayout)
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
    Viewport Viewport = {};
    RasterizerState Rasterizer = {};
    VkFormat ColorAttachmentFormat = {};

    char* Name = "Unknown Pipeline";

  private:
    friend class Pipeline;
    VkDescriptorSetLayout& GlobalDescriptorSetLayout;
    VkDescriptorSetLayout& PassDescriptorSetLayout;
};

struct Pipeline
{
    Pipeline() = default;
    Pipeline(PipelineDesc& desc);

    void cleanup();

    VkPipeline PipelineObject = {};
    VkPipelineLayout PipelineLayout = {};
    char* Name = nullptr;
};

// TODO: Switch naming style to convention (All caps with "_" between words)
enum class TransitionType
{
    ColorAttachment,
    DepthStencilAttachment,
    Present,
    ShaderRead,  // sampled image
    TransferDst, // for upload
    TransferSrc  // for copy
};

// TODO: Refactor (create RenderTargetInfo struct which holds construct info that will be passed in the constructor of RenderTarget)
struct RenderTarget
{
    RenderTarget() = default;
    RenderTarget(VkImage image, VkImageView view, VkFormat Format = VK_FORMAT_UNDEFINED,
                 VkImageLayout layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
                 VkAttachmentLoadOp load = VK_ATTACHMENT_LOAD_OP_CLEAR,
                 VkAttachmentStoreOp store = VK_ATTACHMENT_STORE_OP_STORE, VkClearValue clear = {})
        : Image(image), ImageView(view), Format(Format), ImageLayout(layout), LoadOp(load),
          StoreOp(store), ClearValue(clear)
    {
    }

    VkImage Image = VK_NULL_HANDLE;
    VkImageView ImageView = VK_NULL_HANDLE;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkClearValue ClearValue = {};
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

    RenderTarget ColorAttachment = {};
    RenderTarget DepthAttachment = {};

    bool HasDepth = false;
    bool PrepareForPresent = false;
};

} // namespace niji
