#include <iostream>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "engine/engine.hpp"

#include "app/camera_system.hpp"
#include "app/app.hpp"

int main()
{
    nijiEngine.init();

    auto& app = nijiEngine.ecs.register_system<App>();
    auto& cameraSystem = nijiEngine.ecs.register_system<CameraSystem>();

    nijiEngine.run();

    nijiEngine.cleanup();
    
    return 0;
}