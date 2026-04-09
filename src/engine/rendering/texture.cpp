#include "texture.hpp"

#include <stdexcept>
#include <stb_image.h>
#include <vk_mem_alloc.h>
#include <imgui_impl_vulkan.h>

#include "../core/common.hpp"

#include "engine.hpp"
#include "render_target.hpp"

namespace niji
{

Texture::Texture(const TextureDesc& desc) : Desc(desc)
{
    if (desc.Format == VK_FORMAT_UNDEFINED)
        throw std::runtime_error("Texture creation failed, invalid format!");
    if (desc.Mips < 1)
        throw std::runtime_error("Texture creation failed, invalid amount of mips!");
    if (desc.Layers < 1 || desc.Layers > 6)
        throw std::runtime_error("Texture creation failed, invalid number of layers!");
    if (desc.Type == TextureDesc::TextureType::CUBEMAP && desc.Layers != 6)
        throw std::runtime_error("Cubemaps must have exactly 6 layers!");

    const VkImageCreateFlags flags = desc.Type == TextureDesc::TextureType::CUBEMAP ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

    if (Desc.IsMipMapped)
    {
        Desc.Mips = static_cast<uint32_t>(std::floor(std::log2(std::max(Desc.Width, desc.Height)))) + 1;
        Desc.Usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    nijiEngine.m_context.create_image(Desc.Width, Desc.Height, Desc.Mips, Desc.Layers, Desc.Format, VK_IMAGE_TILING_OPTIMAL,
                                      Desc.Usage, Desc.MemoryUsage, flags, TextureImage, TextureImageAllocation);

    nijiEngine.m_context.transition_image_layout(TextureImage, Desc.Format, VK_IMAGE_LAYOUT_UNDEFINED,
                                                 !Desc.IsReadWrite ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                                                   : VK_IMAGE_LAYOUT_GENERAL,
                                                 Desc.Mips, Desc.Layers);
    if (!Desc.IsReadWrite)
    {
        if (!Desc.Data)
        {
            throw std::runtime_error("2D texture creation failed, no pixel data provided!");
        }

        const VkDeviceSize imageSize = static_cast<VkDeviceSize>(Desc.Width) * Desc.Height * Desc.Channels;

        VkBuffer stagingBuffer = {};
        VmaAllocation stagingAlloc = {};
        nijiEngine.m_context.create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer,
                                           stagingAlloc);

        void* data = nullptr;
        vmaMapMemory(nijiEngine.m_context.m_allocator, stagingAlloc, &data);
        memcpy(data, Desc.Data, static_cast<size_t>(imageSize));
        vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingAlloc);
        stbi_image_free((void*)Desc.Data);

        nijiEngine.m_context.copy_buffer_to_image(stagingBuffer, TextureImage, Desc.Width, Desc.Height, Desc.Layers, 0, 0);

        vmaDestroyBuffer(nijiEngine.m_context.m_allocator, stagingBuffer, stagingAlloc);
    }

    if (!Desc.IsReadWrite)
    {
        if (!Desc.IsMipMapped)
            nijiEngine.m_context.transition_image_layout(TextureImage, Desc.Format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Desc.Mips, Desc.Layers);
        else if (Desc.IsMipMapped)
            nijiEngine.m_context.generateMipmaps(TextureImage, Desc.Format, Desc.Width, Desc.Height, Desc.Mips);
    }

    TextureImageView =
        nijiEngine.m_context.create_image_view(TextureImage, Desc.Format, VK_IMAGE_ASPECT_COLOR_BIT, Desc.Mips, Desc.Layers);

    ImageInfo.imageLayout = !Desc.IsReadWrite ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
    ImageInfo.imageView = TextureImageView;
    ImageInfo.sampler = VK_NULL_HANDLE;

    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_IMAGE, TextureImage, Desc.Name);

    if (Desc.ShowInImGui)
    {
        ImGuiHandle = ImGui_ImplVulkan_AddTexture(nijiEngine.m_context.m_globalSampler.Handle, TextureImageView,
                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

Texture::Texture(const TextureDesc& desc, const std::string& path)
{
    if (desc.Type != TextureDesc::TextureType::CUBEMAP)
        throw std::runtime_error("You used the wrong constructor :/");
    if (desc.Format == VK_FORMAT_UNDEFINED)
        throw std::runtime_error("Texture creation failed, invalid format!");
    if (desc.Mips < 1)
        throw std::runtime_error("Texture creation failed, invalid amount of mips!");
    if (desc.Layers != 6)
        throw std::runtime_error("Cubemaps must have exactly 6 layers!");

    const VkImageCreateFlags flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    nijiEngine.m_context.create_image(desc.Width, desc.Height, desc.Mips, desc.Layers, desc.Format, VK_IMAGE_TILING_OPTIMAL,
                                      desc.Usage, desc.MemoryUsage, flags, TextureImage, TextureImageAllocation);

    nijiEngine.m_context.transition_image_layout(TextureImage, desc.Format, VK_IMAGE_LAYOUT_UNDEFINED,
                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, desc.Mips, desc.Layers);

    if (desc.Mips > 1)
    {
        // Load all faces and mips
        int w, h, n;
        std::vector<std::vector<void*>> cubemapData(desc.Mips, std::vector<void*>(6));
        for (int mip = 0; mip < desc.Mips; ++mip)
        {
            const std::string mipPath = path + "/mip" + std::to_string(mip) + "/";
            const std::string faces[6] = {"px", "nx", "py", "ny", "pz", "nz"};

            for (int face = 0; face < 6; ++face)
            {
                const std::vector<char> file = read_binary_file(mipPath + faces[face] + ".hdr");
                if (file.empty())
                    throw std::runtime_error("Failed to load face for mip " + std::to_string(mip));

                float* pixels = stbi_loadf_from_memory((const stbi_uc*)file.data(), (int)file.size(), &w, &h, &n, 4);
                if (!pixels)
                    throw std::runtime_error("Failed to decode HDR mip " + std::to_string(mip));

                cubemapData[mip][face] = pixels;
            }
        }

        if (cubemapData.empty() || cubemapData.size() != desc.Mips)
            throw std::runtime_error("CubemapMipData must contain data for all mips!");

        for (uint32_t mip = 0; mip < desc.Mips; ++mip)
        {
            if (cubemapData[mip].size() != 6)
                throw std::runtime_error("Each mip must contain 6 face images!");

            const uint32_t mipWidth = std::max(1, desc.Width >> mip);
            const uint32_t mipHeight = std::max(1, desc.Height >> mip);
            const VkDeviceSize mipSize = static_cast<uint64_t>(mipWidth) * mipHeight * desc.Channels * sizeof(float);

            for (uint32_t face = 0; face < 6; ++face)
            {
                VkBuffer stagingBuffer = {};
                VmaAllocation stagingAlloc = {};
                nijiEngine.m_context.create_buffer(mipSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                                                   stagingBuffer, stagingAlloc);

                void* mapped = nullptr;
                vmaMapMemory(nijiEngine.m_context.m_allocator, stagingAlloc, &mapped);
                memcpy(mapped, cubemapData[mip][face], mipSize);
                vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingAlloc);
                stbi_image_free(cubemapData[mip][face]);

                nijiEngine.m_context.copy_buffer_to_image(stagingBuffer, TextureImage, mipWidth, mipHeight, 1, face, mip);

                vmaDestroyBuffer(nijiEngine.m_context.m_allocator, stagingBuffer, stagingAlloc);
            }
        }
    }
    else if (desc.Mips == 1) // Diffuse Cubemap
    {
        int w, h, n;
        const std::string faces[6] = {"px", "nx", "py", "ny", "pz", "nz"};

        for (uint32_t face = 0; face < 6; ++face)
        {
            const std::vector<char> file = read_binary_file(path + "/" + faces[face] + ".hdr");
            if (file.empty())
                throw std::runtime_error("Failed to load face: " + faces[face]);

            float* pixels = stbi_loadf_from_memory((const stbi_uc*)file.data(), (int)file.size(), &w, &h, &n, 4);
            if (!pixels)
                throw std::runtime_error("Failed to decode HDR face: " + faces[face]);

            const uint32_t mipWidth = desc.Width;
            const uint32_t mipHeight = desc.Height;
            const VkDeviceSize mipSize = static_cast<VkDeviceSize>(mipWidth) * mipHeight * desc.Channels * sizeof(float);

            VkBuffer stagingBuffer = {};
            VmaAllocation stagingAlloc = {};
            nijiEngine.m_context.create_buffer(mipSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                                               stagingBuffer, stagingAlloc);

            void* mapped = nullptr;
            vmaMapMemory(nijiEngine.m_context.m_allocator, stagingAlloc, &mapped);
            memcpy(mapped, pixels, mipSize);
            vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingAlloc);
            stbi_image_free(pixels);

            nijiEngine.m_context.copy_buffer_to_image(stagingBuffer, TextureImage, mipWidth, mipHeight, 1, face, 0);

            vmaDestroyBuffer(nijiEngine.m_context.m_allocator, stagingBuffer, stagingAlloc);
        }
    }

    nijiEngine.m_context.transition_image_layout(TextureImage, desc.Format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, desc.Mips, desc.Layers);

    TextureImageView =
        nijiEngine.m_context.create_image_view(TextureImage, desc.Format, VK_IMAGE_ASPECT_COLOR_BIT, desc.Mips, desc.Layers);

    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageInfo.imageView = TextureImageView;
    ImageInfo.sampler = VK_NULL_HANDLE;

    SetObjectName(nijiEngine.m_context.m_device, VK_OBJECT_TYPE_IMAGE, TextureImage, Desc.Name);
}

Texture::Texture(RenderTarget& depthRT)
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

void Texture::cleanup() const
{
    if (Desc.ShowInImGui)
        ImGui_ImplVulkan_RemoveTexture(ImGuiHandle);

    vkDestroyImageView(nijiEngine.m_context.m_device, TextureImageView, nullptr);
    vmaDestroyImage(nijiEngine.m_context.m_allocator, TextureImage, TextureImageAllocation);
}

} // namespace niji