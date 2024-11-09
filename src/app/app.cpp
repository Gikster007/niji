#include "app.hpp"

using namespace molten;

void Engine::run()
{
    m_renderer = Renderer(m_context);

    init_vulkan();
    main_loop();
    cleanup();
}

void Engine::init_vulkan()
{
    m_context.init();
    m_renderer.init();
}

void Engine::main_loop()
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
