#include "material.hpp"

#include <vk_mem_alloc.h>
#include <stb_image.h>

#include "core/context.hpp"

#include "engine.hpp"

using namespace niji;

inline static glm::vec3 ToGLM(const fastgltf::math::nvec3& v)
{
    return glm::vec3(v[0], v[1], v[2]);
}

inline static glm::vec4 ToGLM(const fastgltf::math::nvec4& v)
{
    return glm::vec4(v[0], v[1], v[2], v[3]);
}

Material::Material(fastgltf::Asset& model, fastgltf::Primitive& primitive,
                   std::filesystem::path gltfPath)
{
    // Create Material Data Buffer
    VkDeviceSize bufferSize = sizeof(ModelData);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        ModelData ubo = {};
        BufferDesc bufferDesc = {};
        bufferDesc.IsPersistent = true;
        bufferDesc.Name = "Model Data";
        bufferDesc.Size = sizeof(ModelData);
        bufferDesc.Usage = BufferDesc::BufferUsage::Uniform;
        m_data[i] = Buffer(bufferDesc, &ubo);
    }

    if (!primitive.materialIndex.has_value())
    {
        printf("[Material]: Model has no Material data! \n");
        return;
    }
    auto& material = model.materials[primitive.materialIndex.value()];

    auto loadTexture = [&](const auto& textureInfo) -> std::optional<Texture> {
        size_t textureIndex = {};

        // Base Color, RM, Emissive Textures
        if constexpr (std::is_same_v<decltype(textureInfo), const fastgltf::TextureInfo&>)
        {
            textureIndex = textureInfo.textureIndex;
        }
        else if constexpr (std::is_same_v<decltype(textureInfo),
                                          const fastgltf::NormalTextureInfo&>)
        {
            textureIndex = textureInfo.textureIndex;
        }
        else if constexpr (std::is_same_v<decltype(textureInfo),
                                          const fastgltf::OcclusionTextureInfo&>)
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

            imageData = stbi_load(fullTexturePath.string().c_str(), &width, &height, &channels,
                                  STBI_rgb_alpha);
        }
        else if (std::holds_alternative<fastgltf::sources::Vector>(image.data))
        {
            // Image is embedded as raw bytes
            auto& bufferData = std::get<fastgltf::sources::Vector>(image.data);

            imageData =
                stbi_load_from_memory((stbi_uc*)bufferData.bytes.data(), bufferData.bytes.size(),
                                      &width, &height, &channels, STBI_rgb_alpha);
        }
        else if (std::holds_alternative<fastgltf::sources::BufferView>(image.data))
        {
            // Image is stored in a buffer view (GLB files)
            auto& sourcebufferView = std::get<fastgltf::sources::BufferView>(image.data);
            auto& bufferView = model.bufferViews[sourcebufferView.bufferViewIndex];
            auto& buffer = model.buffers[bufferView.bufferIndex];

            auto& bufferBytes = std::get<fastgltf::sources::Array>(buffer.data);

            imageData =
                stbi_load_from_memory((stbi_uc*)bufferBytes.bytes.data() + bufferView.byteOffset,
                                      bufferView.byteLength, &width, &height, &channels,
                                      STBI_rgb_alpha);
        }

        if (!imageData)
        {
            printf("[Material]: Failed to load image data from file! \n");
            return std::nullopt;
        }

        TextureDesc desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.Channels = 4;
        desc.Data = imageData;
        desc.Format = VK_FORMAT_R8G8B8A8_SRGB;
        desc.MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
        desc.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        Texture finalTexture = Texture(desc);
        return finalTexture;
    };

    // Load all relevant textures
    if (material.pbrData.baseColorTexture.has_value())
        m_materialData.BaseColor = loadTexture(material.pbrData.baseColorTexture.value());

    if (material.normalTexture.has_value())
        m_materialData.NormalTexture = loadTexture(material.normalTexture.value());

    if (material.occlusionTexture.has_value())
        m_materialData.OcclusionTexture = loadTexture(material.occlusionTexture.value());

    if (material.pbrData.metallicRoughnessTexture.has_value())
        m_materialData.RoughMetallic =
            loadTexture(material.pbrData.metallicRoughnessTexture.value());

    if (material.emissiveTexture.has_value())
        m_materialData.Emissive = loadTexture(material.emissiveTexture.value());

    // Create Sampler
    {
        SamplerDesc desc = {};
        desc.MagFilter = SamplerDesc::Filter::LINEAR;
        desc.MinFilter = SamplerDesc::Filter::LINEAR;
        desc.AddressModeU = SamplerDesc::AddressMode::REPEAT;
        desc.AddressModeV = SamplerDesc::AddressMode::REPEAT;
        desc.AddressModeW = SamplerDesc::AddressMode::REPEAT;
        desc.EnableAnisotropy = true;
        desc.MipmapMode = SamplerDesc::MipMapMode::LINEAR;
        
        m_sampler = Sampler(desc);
    }

    std::array<std::optional<Texture>*, 5> textures = {&m_materialData.BaseColor,
                                                       &m_materialData.NormalTexture,
                                                       &m_materialData.OcclusionTexture,
                                                       &m_materialData.RoughMetallic,
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
    m_sampler.cleanup();

    if (m_materialData.BaseColor.has_value())
        m_materialData.BaseColor->cleanup();
    if (m_materialData.Emissive.has_value())
        m_materialData.Emissive->cleanup();
    if (m_materialData.NormalTexture.has_value())
        m_materialData.NormalTexture->cleanup();
    if (m_materialData.OcclusionTexture.has_value())
        m_materialData.OcclusionTexture->cleanup();
    if (m_materialData.RoughMetallic.has_value())
        m_materialData.RoughMetallic->cleanup();

    for (int i = 0; i < m_data.size(); i++)
    {
        m_data[i].cleanup();
    }
}
