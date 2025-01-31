#pragma once

#include <entt/entity/registry.hpp>

// Reference: https://github.com/mxcop/rogue-like/blob/main/src/wyre/core/ecs.h

namespace niji
{

using Entity = entt::entity;
inline constexpr entt::null_t null{};

// ECS System Class
class System
{
  public:
    virtual ~System() = default;

    virtual void update(const float dt)
    {
        // ...
    }

    virtual void render()
    {
        // ...
    }

    virtual void cleanup()
    {

    }
};

// EnTT Wrapper for Entities, Components and Systems
class ECS
{
    friend class Engine;
    ECS();
    ~ECS();

  public:
    entt::registry m_registry = {};

    // Create an entity
    Entity create_entity()
    {
        return m_registry.create();
    }

    // Destroy the given entity
    void destroy_entity(Entity entity);

    // Returns first component of the given type
    template <typename T>
    T& find_component()
    {
        const auto view = m_registry.view<T>();
        const Entity entity = view.front();
        if (entity == niji::null)
            assert(false && "No component matched the given type!");
        return view.get<T>(entity);
    }

    // Returns first entity of the given type
    template <typename T, typename... U>
    Entity find_entity_of_type()
    {
        const auto view = m_registry.view<T, U...>();
        return view.front();
    }

    // Return component of given type, attached to the given entity
    template <typename T>
    T& get_component(const Entity entity)
    {
        return m_registry.get<T>(entity);
    }

    // Return component of given type, attached to the given entity
    template <typename T>
    T* try_get_component(const Entity entity)
    {
        return m_registry.try_get<T>(entity);
    }

    // Add a component (of the given type) to the given entity
    template <typename T, typename... Args>
    decltype(auto) add_component(Entity entity, Args&&... args)
    {
        return m_registry.emplace<T>(entity, std::forward<Args>(args)...);
    }

    // Register a new system
    template <typename T, typename... Args>
    T& register_system(Args&&... args)
    {
        T* system = new T(std::forward<Args>(args)...);
        m_systems.push_back(std::unique_ptr<System>(reinterpret_cast<System*>(system)));
        return *system;
    }

    // Find a system of given type
    template <typename T>
    T& find_system()
    {
        for (auto& s : m_systems)
        {
            T* found = dynamic_cast<T*>(s.get());
            if (found)
                return *found;
        }
        assert(false && "No system of given type could be found!");
    }

    // Find all systems of given type
    template <typename T>
    std::vector<T*> find_systems()
    {
        std::vector<T*> findings = {};
        for (auto& s : m_systems)
        {
            if (T* found = dynamic_cast<T*>(s.get()))
                findings.push_back(found);
        }
        return findings;
    }

  private:
    struct Delete{};

    void systems_update(const float dt);
    void systems_render();
    void systems_cleanup();

    void remove_deleted();
    std::vector<std::unique_ptr<System>> m_systems = {};
};

} // namespace niji