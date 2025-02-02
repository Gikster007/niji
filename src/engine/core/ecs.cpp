#include "ecs.hpp"

using namespace niji;

ECS::ECS() = default;
ECS::~ECS() = default;

void ECS::destroy_entity(Entity entity)
{
    /* Just return if the entity isn't valid */
    if (m_registry.valid(entity) == false)
        return;

    /* Tag entity for deletion */
    m_registry.emplace_or_replace<Delete>(entity);
}

void ECS::systems_update(const float dt)
{
    for (auto& s : m_systems)
        s->update(dt);
}

void ECS::systems_render()
{
    for (auto& s : m_systems)
        s->render();
}

void ECS::systems_cleanup()
{
    for (auto& s : m_systems)
        s->cleanup();
}

void ECS::remove_deleted()
{
    bool queueEmpty = false;
    while (queueEmpty == false)
    {
        const auto del = m_registry.view<Delete>();
        m_registry.destroy(del.begin(), del.end());
        queueEmpty = del.empty();
    }
} 