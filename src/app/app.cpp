#include "app.hpp"

#include <nlohmann/json.hpp>
#include <imgui.h>

#include "../engine/core/components/render-components.hpp"
#include "../engine/core/components/transform.hpp"
#include "../engine/core/envmap.hpp"
#include "../engine/engine.hpp"

using json = nlohmann::json;

App::App()
{
    auto& renderer = nijiEngine.ecs.register_system<niji::Renderer>();

    auto entity = nijiEngine.ecs.create_entity();
    auto& t = nijiEngine.ecs.add_component<niji::Transform>(entity);
    // t.SetScale({1.0f, 1.0f, 1.0f});
    t.SetScale({0.01f, 0.01f, 0.01f});
    t.SetRotation(glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(0.0f), glm::radians(0.0f))));

    /*auto dirLightEntity = nijiEngine.ecs.create_entity();
    auto& dirLight =
        nijiEngine.ecs.add_component<niji::DirectionalLight>(dirLightEntity,
                                                             glm::vec3(-0.4f, -1.0f, -1.0f),
                                                             glm::vec3(1.0f, 1.0f, 1.0f), 2.0f);

    auto pointLightEntity = nijiEngine.ecs.create_entity();
    auto& pointLight =
        nijiEngine.ecs.add_component<niji::PointLight>(pointLightEntity,
                                                       glm::vec3(9.9f, 3.9f, 1.4f),
                                                       glm::vec3(0.0f, 0.45f, 1.0f), 20.0f, 2.0f);

    auto pointLightEntity2 = nijiEngine.ecs.create_entity();
    auto& pointLight2 =
        nijiEngine.ecs.add_component<niji::PointLight>(pointLightEntity2,
                                                       glm::vec3(9.9f, 3.9f, -1.0f),
                                                       glm::vec3(1.0f, 0.59f, 0.0f), 20.0f, 2.0f);

    auto pointLightEntity3 = nijiEngine.ecs.create_entity();
    auto& pointLight3 =
        nijiEngine.ecs.add_component<niji::PointLight>(pointLightEntity3,
                                                       glm::vec3(10.8f, 1.6f, -0.3f),
                                                       glm::vec3(0.6f, 0.1f, 0.1f), 20.0f, 2.0f);*/

    // Load Lights from Fike
    {
        std::ifstream file("assets/lights.json");
        if (!file.is_open())
            return;

        json root;
        file >> root;

        for (auto& j : root["PointLights"])
        {
            niji::PointLight light = j.get<niji::PointLight>();
            auto ent = nijiEngine.ecs.create_entity();
            nijiEngine.ecs.add_component<niji::PointLight>(ent, light);
        }

        for (auto& j : root["DirectionalLights"])
        {
            niji::DirectionalLight light = j.get<niji::DirectionalLight>();
            auto ent = nijiEngine.ecs.create_entity();
            nijiEngine.ecs.add_component<niji::DirectionalLight>(ent, light);
        }
    }

    // m_models.emplace_back(std::make_shared<niji::Model>("assets/DamagedHelmet/DamagedHelmet.glb",
    // entity));
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

// Partly from ChatGPT
static void DrawLightEditor()
{
    // === Point Lights UI ===
    if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int i = 0;
        auto pointLightView = nijiEngine.ecs.m_registry.view<niji::PointLight>();
        for (const auto& [ent, pointLight] : pointLightView.each())
        {
            std::string label = "PointLight " + std::to_string(i);

            if (ImGui::TreeNode(label.c_str()))
            {
                ImGui::DragFloat3(("Position##" + std::to_string(i)).c_str(),
                                  &pointLight.Position[0], 0.1f);
                ImGui::ColorEdit3(("Color##" + std::to_string(i)).c_str(), &pointLight.Color[0]);
                ImGui::DragFloat(("Intensity##" + std::to_string(i)).c_str(), &pointLight.Intensity,
                                 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat(("Range##" + std::to_string(i)).c_str(), &pointLight.Range, 0.1f,
                                 0.0f, 100.0f);

                ImGui::TreePop();
            }
            i++;
        }
    }

    auto dirLightView = nijiEngine.ecs.m_registry.view<niji::DirectionalLight>();
    for (const auto& [ent, dirLight] : dirLightView.each())
    {
        // === Directional Light UI ===
        if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat3("Direction", &dirLight.Direction[0], 0.05f, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color", &dirLight.Color[0]);
            ImGui::DragFloat("Intensity", &dirLight.Intensity, 0.1f, 0.0f, 100.0f);
        }
    }
}

void App::update(float deltaTime)
{
    DrawLightEditor();
}

void App::render()
{
}

void App::cleanup()
{
    // Save Lights to File
    {
        json root;

        auto pointView = nijiEngine.ecs.m_registry.view<niji::PointLight>();
        for (const auto& [ent, light] : pointView.each())
            root["PointLights"].push_back(light);

        auto dirView = nijiEngine.ecs.m_registry.view<niji::DirectionalLight>();
        for (const auto& [ent, light] : dirView.each())
            root["DirectionalLights"].push_back(light);

        std::ofstream file("assets/lights.json");
        if (file.is_open())
            file << root.dump(4); // pretty print
    }

    for (auto& model : m_models)
    {
        // Hacky af but if it works, it works
        model->~Model();
    }
    m_envmap.cleanup();
}
