#include "engine.hpp"

#include <chrono>

using namespace niji;

void Engine::run()
{
    update();
}

Engine::Engine() : ecs(*new ECS()), m_context(*new Context())//, m_renderer(*new Renderer(m_context))
{
}

Engine::~Engine()
{
    delete &ecs;
    delete &m_context;
}

void Engine::init()
{
    m_context.init();
    //m_renderer.init();

    //ecs.register_system<Renderer>(m_context);
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

        //m_renderer.update(dt);

        ecs.systems_update(dt);
        ecs.remove_deleted();

        ecs.systems_render();

        //m_renderer.render();

        time = ctime;
    }
    vkDeviceWaitIdle(m_context.m_device);
}

void Engine::cleanup()
{
    //m_renderer.cleanup();
    ecs.systems_cleanup();
    m_context.cleanup();
}

Engine nijiEngine{};