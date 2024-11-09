#include "app.hpp"

using namespace molten;

void App::run()
{
    m_renderer = Renderer(m_context);

    init_vulkan();
    main_loop();
    cleanup();
}

void App::init_vulkan()
{
    m_context.init();
    m_renderer.init();
}

void App::main_loop()
{
    while (!glfwWindowShouldClose(m_context.m_window))
    {
        glfwPollEvents();
        m_renderer.draw();
    }
    vkDeviceWaitIdle(m_context.m_device);
}

void App::cleanup()
{
    m_renderer.cleanup();
    m_context.cleanup();
}
