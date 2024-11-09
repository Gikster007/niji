#include "engine.hpp"

using namespace molten;

void Engine::run()
{
    m_renderer = Renderer(m_context);

    init();
    update();
    cleanup();
}

void Engine::init()
{
    m_context.init();
    m_renderer.init();
}

void Engine::update()
{
    while (!glfwWindowShouldClose(m_context.m_window))
    {
        glfwPollEvents();
        m_renderer.draw();
    }
    vkDeviceWaitIdle(m_context.m_device);
}

void Engine::cleanup()
{
    m_renderer.cleanup();
    m_context.cleanup();
}
