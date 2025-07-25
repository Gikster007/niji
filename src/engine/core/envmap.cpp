#include "envmap.hpp"

#include <stdexcept>
#include <fstream>

#include <vk_mem_alloc.h>
#include <stb_image.h>

using namespace niji;

Envmap::Envmap(const std::string& path)
{
    LoadSpecular(path + "/specular", 7);
    LoadDiffuse(path + "/diffuse");
    LoadLUT(path + "/specular");
}

Envmap::~Envmap()
{
}

void Envmap::LoadSpecular(const std::string& path, const int mips)
{
    if (mips <= 0)
        throw std::runtime_error("[Envmap] Number of mips must be larger than 0!");

    int w = -1, h = -1, n = -1; /* Load the first cubemap face to get its width and height */
    const std::vector<char> first_blob =
        read_binary_file(path + "/mip0/px.hdr");
    if (first_blob.empty())
        throw std::runtime_error("[Envmap] Failed to load cubemap face!");
    stbi_image_free(stbi_loadf_from_memory((const stbi_uc*)first_blob.data(),
                                           (int)first_blob.size(), &w, &h, &n, 4));

    /* Build the cubemap texture spec */
    TextureDesc desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.Channels = 4;
    desc.Type = TextureDesc::TextureType::CUBEMAP;
    desc.Mips = mips;
    desc.Layers = 6; // Number of Faces
    desc.Format = VK_FORMAT_R32G32B32A32_SFLOAT;
    desc.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    desc.MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    //desc.DebugName = "Environment Specular Map";

    m_specularCubemap = Texture(desc, path);

    SamplerDesc samplerDesc = {};
    samplerDesc.MagFilter = SamplerDesc::Filter::LINEAR;
    samplerDesc.MinFilter = SamplerDesc::Filter::LINEAR;
    samplerDesc.AddressModeU = SamplerDesc::AddressMode::EDGE_CLAMP;
    samplerDesc.AddressModeV = SamplerDesc::AddressMode::EDGE_CLAMP;
    samplerDesc.AddressModeW = SamplerDesc::AddressMode::EDGE_CLAMP;
    samplerDesc.EnableAnisotropy = true;
    samplerDesc.MipmapMode = SamplerDesc::MipMapMode::LINEAR;
    samplerDesc.MaxMips = mips;

    m_sampler = Sampler(samplerDesc);

    printf("\n[Envmap] Specular Map has loaded Successfully! \n");
}

void Envmap::LoadDiffuse(const std::string& path)
{
    int w = -1, h = -1, n = -1; /* Load the first cubemap face to get its width and height */
    const std::vector<char> first_blob = read_binary_file(path + "/px.hdr");
    if (first_blob.empty())
        throw std::runtime_error("[Envmap] Failed to load diffuse cubemap face!");
    stbi_image_free(stbi_loadf_from_memory((const stbi_uc*)first_blob.data(),
                                           (int)first_blob.size(), &w, &h, &n, 4));

    /* Build the cubemap texture spec */
    TextureDesc desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.Channels = 4;
    desc.Type = TextureDesc::TextureType::CUBEMAP;
    desc.Mips = 1;
    desc.Layers = 6; // Number of Faces
    desc.Format = VK_FORMAT_R32G32B32A32_SFLOAT;
    desc.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    desc.MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    // desc.DebugName = "Environment Diffuse Map";

    m_diffuseCubemap = Texture(desc, path);

    printf("\n[Envmap] Diffuse Map has loaded Successfully! \n");
}

void Envmap::LoadLUT(const std::string& path)
{
    const std::string filename = path + "/brdf_lut.png";
    const std::vector<char> blob = read_binary_file(filename);

    int width = -1, height = -1, channels = -1;
    auto* data =
        (unsigned char*)stbi_load_from_memory((const stbi_uc*)blob.data(), (int)blob.size(), &width,
                                              &height, &channels, 4);

    TextureDesc desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.Channels = 4;
    desc.Data = data;
    desc.Type = TextureDesc::TextureType::TEXTURE_2D;
    desc.Mips = 1;
    desc.Layers = 1; // Number of Faces
    desc.Format = VK_FORMAT_R16G16_SFLOAT;
    desc.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    desc.MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    //desc.DebugName = "Environment LUT";

    m_brdfTexture = Texture(desc);

    printf("\n[Envmap] LUT has loaded Successfully! \n");
}

void Envmap::cleanup()
{
    m_specularCubemap.cleanup();
    m_diffuseCubemap.cleanup();
    m_brdfTexture.cleanup();
    m_sampler.cleanup();
}