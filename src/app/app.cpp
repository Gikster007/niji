#include "app.hpp"

#include "../engine/core/components/render-components.hpp"
#include "../engine/core/components/transform.hpp"
#include "../engine/core/envmap.hpp"
#include "../engine/engine.hpp"

App::App()
{
    auto& renderer =  nijiEngine.ecs.register_system<niji::Renderer>();

    auto entity = nijiEngine.ecs.create_entity();
    auto& t = nijiEngine.ecs.add_component<niji::Transform>(entity);
    // t.SetScale({1.0f, 1.0f, 1.0f});
    t.SetScale({0.01f, 0.01f, 0.01f});
    t.SetRotation(glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(0.0f), glm::radians(0.0f))));

    auto dirLightEntity = nijiEngine.ecs.create_entity();
    auto& dirLight =
        nijiEngine.ecs.add_component<niji::DirectionalLight>(dirLightEntity, glm::vec3(-0.4f, -1.0f, -1.0f),
                                                             glm::vec3(1.0f, 1.0f, 1.0f), 2.0f);

    auto pointLightEntity = nijiEngine.ecs.create_entity();
    auto& pointLight =
        nijiEngine.ecs.add_component<niji::PointLight>(pointLightEntity,
                                                             glm::vec3(-2.0f, 1.0f, 0.0f),
                                                             glm::vec3(1.0f, 0.0f, 0.0f), 2.0f, 1.0f);

    auto pointLightEntity2 = nijiEngine.ecs.create_entity();
    auto& pointLight2 =
        nijiEngine.ecs.add_component<niji::PointLight>(pointLightEntity2,
                                                       glm::vec3(-4.0f, 2.0f, -3.0f),
                                                       glm::vec3(0.0f, 1.0f, 0.0f), 20.0f, 10.0f);

    //m_models.emplace_back(std::make_shared<niji::Model>("assets/DamagedHelmet/DamagedHelmet.glb", entity));
    m_models.emplace_back(std::make_shared<niji::Model>("assets/Sponza/Sponza.gltf", entity));

    m_envmap = niji::Envmap("assets/environments/footprint_court");
    renderer.set_envmap(m_envmap);

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
    m_envmap.cleanup();
}
