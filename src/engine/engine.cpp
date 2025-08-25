#include "engine.hpp"

#include <chrono>

#include <glm/gtc/constants.hpp>

using namespace niji;

void Engine::run()
{
    update();
}

Engine::Engine() : ecs(*new ECS()), m_context(*new Context()), m_editor(*new Editor())
{
}

Engine::~Engine()
{
    delete &ecs;
    delete &m_context;
    delete &m_editor;
}

void Engine::init()
{
    m_context.init();
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
    ecs.systems_cleanup();
    m_context.cleanup();
}

void Engine::add_line(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& color)
{
    if (m_debugLines.size() <= MAX_DEBUG_LINES - 2)
    {
        m_debugLines.push_back({p0, color});
        m_debugLines.push_back({p1, color});
    }
}

void Engine::add_sphere(const glm::vec3& center, const float& radius, const glm::vec3& color, int segments)
{
    const float step = glm::two_pi<float>() / segments;

    // XY circle
    for (int i = 0; i < segments; ++i)
    {
        float a0 = i * step;
        float a1 = (i + 1) * step;

        glm::vec3 p0 = center + radius * glm::vec3(cos(a0), sin(a0), 0.0f);
        glm::vec3 p1 = center + radius * glm::vec3(cos(a1), sin(a1), 0.0f);

        add_line(p0, p1, color);
    }

    // XZ circle
    for (int i = 0; i < segments; ++i)
    {
        float a0 = i * step;
        float a1 = (i + 1) * step;

        glm::vec3 p0 = center + radius * glm::vec3(cos(a0), 0.0f, sin(a0));
        glm::vec3 p1 = center + radius * glm::vec3(cos(a1), 0.0f, sin(a1));

        add_line(p0, p1, color);
    }

    // YZ circle
    for (int i = 0; i < segments; ++i)
    {
        float a0 = i * step;
        float a1 = (i + 1) * step;

        glm::vec3 p0 = center + radius * glm::vec3(0.0f, cos(a0), sin(a0));
        glm::vec3 p1 = center + radius * glm::vec3(0.0f, cos(a1), sin(a1));

        add_line(p0, p1, color);
    }
}

Engine nijiEngine{};