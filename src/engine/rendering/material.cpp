#include "material.hpp"

#include "engine.hpp"
#include "core/context.hpp"

#include <stb_image.h>
#include <vk_mem_alloc.h>

using namespace niji;

Material::Material(fastgltf::Asset& model, fastgltf::Primitive& primitive, NijiUBO& ubo)
{
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

        NijiTexture finalTexture = nijiEngine.m_context.create_texture_image(imageData, width, height, channels);
        nijiEngine.m_context.create_texture_image_view(finalTexture);
        return finalTexture;
    };

    // Load all relevant textures
    if (material.pbrData.baseColorTexture.has_value())
        m_materialData.BaseColor = loadTexture(material.pbrData.baseColorTexture.value());

    if (material.normalTexture.has_value())
        m_materialData.NormalTexture = loadTexture(material.normalTexture);

    if (material.occlusionTexture.has_value())
        m_materialData.OcclusionTexture = loadTexture(material.occlusionTexture);

    if (material.pbrData.metallicRoughnessTexture.has_value())
        m_materialData.RoughMetallic = loadTexture(material.pbrData.metallicRoughnessTexture);

    if (material.emissiveTexture.has_value())
        m_materialData.Emissive = loadTexture(material.emissiveTexture);

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

    // Create Descriptor Layouts for Mesh-Specific Textures
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,
                                                                samplerLayoutBinding};

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(nijiEngine.m_context.m_device, &layoutInfo, nullptr,
                                        &m_descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("[Material]: Failed to Create Descriptor Set Layout");
    }

    // Create Descriptor Pool for Mesh-Specific Textures
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(nijiEngine.m_context.m_device, &poolInfo, nullptr, &m_descriptorPool) !=
            VK_SUCCESS)
            throw std::runtime_error("Failed to Create Descriptor Pool!");
    }

    // Create Descriptor Sets for Mesh-Specific Textures
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(nijiEngine.m_context.m_device, &allocInfo, m_descriptorSets.data()) !=
            VK_SUCCESS)
            throw std::runtime_error("Failed to Allocate Descriptor Sets!");

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = ubo.UniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_materialData.BaseColor.value().TextureImageView;
            imageInfo.sampler = m_sampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pTexelBufferView = nullptr;
            descriptorWrites[0].pImageInfo = nullptr;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = nullptr;
            descriptorWrites[1].pTexelBufferView = nullptr;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(nijiEngine.m_context.m_device,
                                   static_cast<uint32_t>(descriptorWrites.size()),
                                   descriptorWrites.data(), 0, nullptr);
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

    vkDestroyDescriptorPool(nijiEngine.m_context.m_device, m_descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(nijiEngine.m_context.m_device, m_descriptorSetLayout, nullptr);
}
