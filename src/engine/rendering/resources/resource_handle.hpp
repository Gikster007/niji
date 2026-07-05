#pragma once

// Inspiration: https://github.com/mxcop/graphite/blob/thermite-engine/src/core/graphite/resources/handle.hh

namespace niji
{

enum class ResourceType : uint32_t
{
    Invalid = 0u,
    RenderTarget = 1u,
    Buffer = 2u,
    Texture = 3u,
    Sampler = 4u,
};

struct ResourceHandle
{
    ResourceHandle() = default;

    ResourceHandle(uint32_t index, ResourceType type) : Index(index), Type(type)
    {
    }

    inline bool is_valid() const
    {
        return Index != 0u && Type != ResourceType::Invalid;
    }
    inline uint32_t raw() const
    {
        return Index | (static_cast<uint32_t>(Type) << 28);
    }

    /* To access the constructor. */
    template <typename, typename, ResourceType>
    friend class Pool;

    uint32_t Index : 28;
    ResourceType Type : 4;
};

struct RenderTargetHandle : ResourceHandle
{
};
struct BufferHandle : ResourceHandle
{
};
struct TextureHandle : ResourceHandle
{
};
struct SamplerHandle : ResourceHandle
{
};

} // namespace niji