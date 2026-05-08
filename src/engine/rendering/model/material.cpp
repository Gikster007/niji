#include "material.hpp"

#include <vk_mem_alloc.h>
#include <stb_image.h>

#include "core/context.hpp"

#include "engine.hpp"
#include "rendering/renderer.hpp"
#include "rendering/resource_bank.hpp"

using namespace niji;

inline static glm::vec3 ToGLM(const fastgltf::math::nvec3& v)
{
    return glm::vec3(v[0], v[1], v[2]);
}

inline static glm::vec4 ToGLM(const fastgltf::math::nvec4& v)
{
    return glm::vec4(v[0], v[1], v[2], v[3]);
}

Material::Material(fastgltf::Asset& model, fastgltf::Primitive& primitive, std::filesystem::path gltfPath)
{
    // Create Material Data Buffer
    BufferDesc bufferDesc = {};
    bufferDesc.Name = "Model Data";
    bufferDesc.Size = sizeof(ModelData);
    bufferDesc.Usage = BufferUsage::Uniform | BufferUsage::TransferDst;
    m_data = nijiEngine.m_renderer.m_resourceBank.create_buffer(bufferDesc);

    if (!primitive.materialIndex.has_value())
    {
        printf("[Material]: Model has no Material data! \n");
        return;
    }
    auto& material = model.materials[primitive.materialIndex.value()];

    int largestWidth = 1, largestHeight = 1;

    auto loadTexture = [&](const auto& textureInfo, bool isLinear) -> std::optional<TextureHandle> {
        size_t textureIndex = {};

        // Base Color, RM, Emissive Textures
        if constexpr (std::is_same_v<decltype(textureInfo), const fastgltf::TextureInfo&>)
        {
            textureIndex = textureInfo.textureIndex;
        }
        else if constexpr (std::is_same_v<decltype(textureInfo), const fastgltf::NormalTextureInfo&>)
        {
            textureIndex = textureInfo.textureIndex;
        }
        else if constexpr (std::is_same_v<decltype(textureInfo), const fastgltf::OcclusionTextureInfo&>)
        {
            textureIndex = textureInfo.textureIndex;
        }

        if (textureIndex >= model.textures.size())
            return std::nullopt;

        auto& gltfTexture = model.textures[textureIndex];
        if (!gltfTexture.imageIndex.has_value())
            return std::nullopt;

        size_t imageIndex = gltfTexture.imageIndex.value();
        if (imageIndex >= model.images.size())
            return std::nullopt;

        auto& image = model.images[imageIndex];

        int width = -1, height = -1, channels = -1;
        unsigned char* imageData = nullptr;

        // Handle different image sources
        if (std::holds_alternative<fastgltf::sources::URI>(image.data))
        {
            // Image is stored as a URI (external file)
            std::filesystem::path baseDir = gltfPath.parent_path();
            auto& uri = std::get<fastgltf::sources::URI>(image.data);
            std::filesystem::path fullTexturePath = baseDir / uri.uri.fspath();

            imageData = stbi_load(fullTexturePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        }
        else if (std::holds_alternative<fastgltf::sources::Vector>(image.data))
        {
            // Image is embedded as raw bytes
            auto& bufferData = std::get<fastgltf::sources::Vector>(image.data);

            imageData = stbi_load_from_memory((stbi_uc*)bufferData.bytes.data(), bufferData.bytes.size(), &width, &height,
                                              &channels, STBI_rgb_alpha);
        }
        else if (std::holds_alternative<fastgltf::sources::BufferView>(image.data))
        {
            // Image is stored in a buffer view (GLB files)
            auto& sourcebufferView = std::get<fastgltf::sources::BufferView>(image.data);
            auto& bufferView = model.bufferViews[sourcebufferView.bufferViewIndex];
            auto& buffer = model.buffers[bufferView.bufferIndex];

            auto& bufferBytes = std::get<fastgltf::sources::Array>(buffer.data);

            imageData = stbi_load_from_memory((stbi_uc*)bufferBytes.bytes.data() + bufferView.byteOffset, bufferView.byteLength,
                                              &width, &height, &channels, STBI_rgb_alpha);
        }

        if (!imageData)
        {
            printf("[Material]: Failed to load image data from file! \n");
            return std::nullopt;
        }

        largestWidth = width > largestWidth ? width : largestWidth;
        largestHeight = height > largestHeight ? height : largestHeight;

        TextureDesc desc = {};
        desc.Size = {(uint32_t)width, (uint32_t)height, 0u};
        desc.Format = isLinear ? TextureFormat::RGBA8Unorm : TextureFormat::RGBA8Srgb;
        desc.Usage = TextureUsage::TransferDst | TextureUsage::Sampled;

        TextureHandle finalTexture = nijiEngine.m_renderer.m_resourceBank.create_texture(desc);
        nijiEngine.m_renderer.m_resourceBank.upload_texture(finalTexture, imageData, width * height *4u);
        return finalTexture;
    };

    // Load all relevant textures
    if (material.pbrData.baseColorTexture.has_value())
        m_materialData.BaseColor = loadTexture(material.pbrData.baseColorTexture.value(), false);

    if (material.normalTexture.has_value())
        m_materialData.NormalTexture = loadTexture(material.normalTexture.value(), true);

    if (material.occlusionTexture.has_value())
        m_materialData.OcclusionTexture = loadTexture(material.occlusionTexture.value(), true);

    if (material.pbrData.metallicRoughnessTexture.has_value())
        m_materialData.RoughMetallic = loadTexture(material.pbrData.metallicRoughnessTexture.value(), true);

    if (material.emissiveTexture.has_value())
        m_materialData.Emissive = loadTexture(material.emissiveTexture.value(), false);

    // Create Sampler
    {
        SamplerDesc desc = {};
        desc.MagFilter = SamplerDesc::Filter::LINEAR;
        desc.MinFilter = SamplerDesc::Filter::LINEAR;
        desc.AddressModeU = SamplerDesc::AddressMode::REPEAT;
        desc.AddressModeV = SamplerDesc::AddressMode::REPEAT;
        desc.AddressModeW = SamplerDesc::AddressMode::REPEAT;
        desc.EnableAnisotropy = true;
        desc.MaxMips = static_cast<uint32_t>(std::floor(std::log2(std::max(largestWidth, largestHeight)))) + 1;
        desc.MipmapMode = SamplerDesc::MipMapMode::LINEAR;

        m_sampler = nijiEngine.m_renderer.m_resourceBank.create_sampler(desc);
    }

    std::array<std::optional<TextureHandle>*, 5> textures = {&m_materialData.BaseColor, &m_materialData.NormalTexture,
                                                       &m_materialData.OcclusionTexture, &m_materialData.RoughMetallic,
                                                       &m_materialData.Emissive};

    m_materialInfo.HasEmissiveMap = m_materialData.Emissive.has_value();
    m_materialInfo.HasMetallicMap = m_materialData.RoughMetallic.has_value();
    m_materialInfo.HasRoughnessMap = m_materialData.RoughMetallic.has_value();
    m_materialInfo.HasNormalMap = m_materialData.NormalTexture.has_value();

    m_materialInfo.AlbedoFactor = ToGLM(material.pbrData.baseColorFactor);
    m_materialInfo.EmissiveFactor = glm::vec4(ToGLM(material.emissiveFactor), 0.0f);
    m_materialInfo.RoughnessFactor = material.pbrData.roughnessFactor;
    m_materialInfo.MetallicFactor = material.pbrData.metallicFactor;
}

void Material::cleanup()
{
    nijiEngine.m_renderer.m_resourceBank.destroy(m_sampler);

    if (m_materialData.BaseColor.has_value())
        nijiEngine.m_renderer.m_resourceBank.destroy(m_materialData.BaseColor.value());
    if (m_materialData.Emissive.has_value())
        nijiEngine.m_renderer.m_resourceBank.destroy(m_materialData.Emissive.value());
    if (m_materialData.NormalTexture.has_value())
        nijiEngine.m_renderer.m_resourceBank.destroy(m_materialData.NormalTexture.value());
    if (m_materialData.OcclusionTexture.has_value())
        nijiEngine.m_renderer.m_resourceBank.destroy(m_materialData.OcclusionTexture.value());
    if (m_materialData.RoughMetallic.has_value())
        nijiEngine.m_renderer.m_resourceBank.destroy(m_materialData.RoughMetallic.value());

    nijiEngine.m_renderer.m_resourceBank.destroy(m_data);
}
