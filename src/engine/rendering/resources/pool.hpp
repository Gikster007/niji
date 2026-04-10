#pragma once

#include "resource_handle.hpp"

namespace niji
{

template <typename Resource, typename RHandle>
struct PoolPair
{
    PoolPair(RHandle handle, Resource& data) : Handle(handle), Data(data) {};

    RHandle Handle {};
    Resource& Data;
};

// Simple Stack Pool Allocator
// Inspiration: https://github.com/mxcop/graphite/blob/thermite-engine/src/core/graphite/resources/stock.hh

template <typename Resource, typename Handle, ResourceType Type>
class Pool
{
  public:
    Pool() = default;
    ~Pool()
    {
        destroy();
    }

    // No Copy
    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    Pool(const uint32_t size) : m_pool(new Resource[size]), m_stack(new Handle[size]), m_capacity(size)
    {
        // Init Handles
        for (uint32_t i = 0u; i < size; i++)
            m_stack[i] = reinterpret_cast<Handle&>(ResourceHandle(i + 1u, Type));
    };

    void init(const uint32_t size)
    {
        // Clean Existing Data
        destroy();

        m_pool = new Resource[size];
        m_stack = new Handle[size];
        m_capacity = size;

        // Init Handles
        for (uint32_t i = 0u; i < size; i++)
            m_stack[i] = reinterpret_cast<Handle&>(ResourceHandle(i + 1u, Type));
    }

    // Pop New Resource Off of Stack
    PoolPair<Resource, Handle> pop()
    {
        assert(m_ptr < m_capacity && "pool overflow!");

        const Handle handle = m_stack[m_ptr];
        m_ptr++; // update pointer to next available handle in the stack

        Resource& resource = m_pool[handle.index - 1u];

        return PoolPair(handle, resource);
    }

    // Push a Resource Onto The Stack
    Resource& push(Handle& handle)
    {
        assert(handle.is_valid());
        assert(m_ptr > 0 && "pool underflow!");

        m_ptr--; // update pointer to free slot in the stack
        m_stack[m_ptr] = handle;
        Resource& resource = m_pool[handle.index - 1u];

        handle = Handle(); // reset handle

        return resource;
    }

    void destroy()
    {
        delete[] m_pool;
        delete[] m_stack;

        m_pool = nullptr;
        m_stack = nullptr;

        m_ptr = 0u;
        m_capacity = 0u;
    }

    // Get Resource From Handle
    Resource& get(ResourceHandle handle)
    {
        return m_pool[handle.index - 1u];
    }

    // Get Resource From Handle
    const Resource& get(ResourceHandle handle) const
    {
        return m_pool[handle.index - 1u];
    }

  private:
    Resource* m_pool = nullptr;
    Handle* m_stack = nullptr;
    uint32_t m_ptr = 0u;
    uint32_t m_capacity = 0u;
};

} // namespace niji