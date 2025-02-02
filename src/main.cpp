#include <iostream>

#include "engine/engine.hpp"
#include "rendering/model.hpp"
#include "core/components/transform.hpp"

int main()
{
    nijiEngine.init();

    auto entity = nijiEngine.ecs.create_entity();
    auto& t = nijiEngine.ecs.add_component<niji::Transform>(entity);
    t.SetScale({1.0f, 1.0f, 1.0f});

    auto model = niji::Model("assets/cube/cube.gltf", entity);

    nijiEngine.run();

    nijiEngine.cleanup();
    
    return 0;
}