#include <iostream>

#define VMA_IMPLEMENTATION
#define DEBUG_ALLOCATIONS 0
#include <vk_mem_alloc.h>

#include "core/components/transform.hpp"
#include "rendering/model/model.hpp"
#include "engine/engine.hpp"

int main()
{
    nijiEngine.init();

    auto entity = nijiEngine.ecs.create_entity();
    auto& t = nijiEngine.ecs.add_component<niji::Transform>(entity);
    t.SetScale({1.0f, 1.0f, 1.0f});
    t.SetRotation(glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(0.0f), glm::radians(0.0f))));

    //auto model = std::make_shared<niji::Model>("assets/cube/cube.gltf", entity);
    auto model = std::make_shared<niji::Model>("assets/DamagedHelmet/DamagedHelmet.glb", entity);

    auto& renderer = nijiEngine.ecs.register_system<niji::Renderer>();

    nijiEngine.run();

    nijiEngine.cleanup();
    
    return 0;
}