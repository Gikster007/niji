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
    } MagFilter = Filter::LINEAR;
    Filter MinFilter = Filter::LINEAR;

    enum class AddressMode
    {
        NONE,
        REPEAT,
        MIRRORED_REPEAT,
        EDGE_CLAMP,
        BORDER_CLAMP,
        MIRRORED_EDGE_CLAMP
    } AddressModeU = AddressMode::REPEAT;
    AddressMode AddressModeV = AddressMode::REPEAT;
    AddressMode AddressModeW = AddressMode::REPEAT;

    enum class MipMapMode
    {
        NONE,
        NEAREST,
        LINEAR
    } MipmapMode = MipMapMode::NEAREST;

    std::string Name = "Unknown Sampler";

    uint32_t MaxMips = 0;
    bool EnableAnisotropy = false;
};

// Sampler Resource
struct Sampler
{
    Sampler() = default;

    //void cleanup() const;

    VkSampler Object = {};
};

} // namespace niji