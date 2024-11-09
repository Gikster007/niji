#include "app.hpp"

using namespace molten;

void App::run()
{
    m_renderer = Renderer(std::make_shared<Context>(m_context));

    init_vulkan();
    main_loop();
    cleanup();
}

void App::init_vulkan()
{
    m_context.init();
    m_renderer.init();

    //create_instance();
    //setup_debug_messenger();
    //create_surface();
    //pick_physical_device();
    //create_logical_device();
    //create_swap_chain();
    //create_image_views();
    //create_graphics_pipeline();
    //create_command_pool();
    //create_command_buffers();
    //create_sync_objects();
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

    //cleanup_swap_chain();

    /*vkDestroyPipeline(m_device, m_graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_device, m_render_finished_semaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
        vkDestroyFence(m_device, m_in_flight_fences[i], nullptr);
    }*/

    /*vkDestroyCommandPool(m_device, m_command_pool, nullptr);*/

    /*vkDestroyDevice(m_device, nullptr);*/

    /*if (ENABLE_VALIDATION_LAYERS)
        DestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);*/

    /*vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);*/

    /*glfwDestroyWindow(m_window);
    glfwTerminate();*/
}
