#pragma once

namespace niji
{

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

} // namespace niji