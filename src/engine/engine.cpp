#include "engine.hpp"

#include <chrono>

using namespace niji;

void Engine::run()
{
    update();
}

void Engine::init()
{
    m_context.init();

    ecs.register_system<Renderer>(m_context);
}

void Engine::update()
{
    auto time = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(m_context.m_window))
    {
        glfwPollEvents();

        auto ctime = std::chrono::high_resolution_clock::now();
        auto elapsed = ctime - time;
        float dt =
            (float)((double)std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() /
                    1000000.0);

        ecs.systems_update(dt);
        ecs.remove_deleted();

        ecs.systems_render();

        time = ctime;
    }
    vkDeviceWaitIdle(m_context.m_device);
}

void Engine::cleanup()
{
    ecs.~ECS();
    m_context.cleanup();
}

Engine nijiEngine = {};