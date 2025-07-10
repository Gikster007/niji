#include "app.hpp"

#include "../engine/core/components/transform.hpp"
#include "../engine/engine.hpp"

App::App()
{
    auto entity = nijiEngine.ecs.create_entity();
    auto& t = nijiEngine.ecs.add_component<niji::Transform>(entity);
    // t.SetScale({1.0f, 1.0f, 1.0f});
    t.SetScale({0.01f, 0.01f, 0.01f});
    t.SetRotation(glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(0.0f), glm::radians(0.0f))));

    m_models.emplace_back(std::make_shared<niji::Model>("assets/DamagedHelmet/DamagedHelmet.glb", entity));
    //m_models.emplace_back(std::make_shared<niji::Model>("assets/Sponza/Sponza.gltf", entity));

    for (const auto& model : m_models)
    {
        model->Instantiate();
    }
}

App::~App()
{
    
}

void App::update(float deltaTime)
{
}

void App::render()
{
}

void App::cleanup()
{
    for (auto& model : m_models)
    {
        // Hacky af but if it works, it works
        model->~Model();
    }
}
