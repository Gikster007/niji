#include "material.hpp"

#include "engine.hpp"
#include "core/context.hpp"

#include <stb_image.h>

using namespace niji;

Material::Material(fastgltf::Asset& model, fastgltf::Primitive& primitive)
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

    auto loadTexture = [&](const auto& textureInfo) -> std::optional<NijiTexture> {
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
            auto& uri = std::get<fastgltf::sources::URI>(image.data);

            imageData = stbi_load(uri.uri.c_str(), &width, &height, &channels, STBI_rgb_alpha);
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

        NijiTexture finalTexture =
            nijiEngine.m_context.create_texture_image(imageData, width, height, channels);
        nijiEngine.m_context.create_texture_image_view(finalTexture);
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
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(nijiEngine.m_context.m_physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(nijiEngine.m_context.m_device, &samplerInfo, nullptr, &m_sampler) !=
            VK_SUCCESS)
            throw std::runtime_error("[Material]: Failed to Create Texture Sampler!");
    }

    std::array<std::optional<NijiTexture>*, 5> textures = {&m_materialData.BaseColor,
                                                           &m_materialData.NormalTexture,
                                                           &m_materialData.OcclusionTexture,
                                                           &m_materialData.RoughMetallic,
                                                           &m_materialData.Emissive};

    for (auto* textureOpt : textures)
    {
        if (textureOpt->has_value())
        {
            auto& texture = textureOpt->value();
            texture.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            texture.ImageInfo.imageView = texture.TextureImageView;
            texture.ImageInfo.sampler = VK_NULL_HANDLE;
        }
    }
}

void Material::cleanup()
{
    vkDestroySampler(nijiEngine.m_context.m_device, m_sampler, nullptr);

    nijiEngine.m_context.cleanup_texture(m_materialData.BaseColor.value());
    nijiEngine.m_context.cleanup_texture(m_materialData.Emissive.value());
    nijiEngine.m_context.cleanup_texture(m_materialData.NormalTexture.value());
    nijiEngine.m_context.cleanup_texture(m_materialData.OcclusionTexture.value());
    nijiEngine.m_context.cleanup_texture(m_materialData.RoughMetallic.value());

    //vkDestroyDescriptorPool(nijiEngine.m_context.m_device, m_descriptorPool, nullptr);
    //
    //vkDestroyDescriptorSetLayout(nijiEngine.m_context.m_device, m_descriptorSetLayout, nullptr);
}
