#include "app.hpp"

#include <nlohmann/json.hpp>
#include <imgui.h>

#include "../engine/core/components/render-components.hpp"
#include "../engine/core/components/transform.hpp"
#include "../engine/core/envmap.hpp"
#include "../engine/engine.hpp"

using json = nlohmann::json;

static void LoadLights()
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

App::App()
{
    auto& renderer = nijiEngine.ecs.register_system<niji::Renderer>();

    auto entity = nijiEngine.ecs.create_entity();
    auto& t = nijiEngine.ecs.add_component<niji::Transform>(entity);
    // t.SetScale({1.0f, 1.0f, 1.0f});
    t.SetScale({0.01f, 0.01f, 0.01f});
    t.SetRotation(glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(0.0f), glm::radians(0.0f))));

    // Load Lights from File
    LoadLights();

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
    ImGui::Begin("Light Editor");

    // === Point Lights ===
    if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Add new point light button
        if (ImGui::Button("Add Point Light"))
        {
            auto ent = nijiEngine.ecs.create_entity();
            nijiEngine.ecs.add_component<niji::PointLight>(ent, glm::vec3(0.0f), glm::vec3(1.0f),
                                                           10.0f, 5.0f);
        }

        std::vector<entt::entity> toRemove = {};

        int i = 0;
        auto pointLightView = nijiEngine.ecs.m_registry.view<niji::PointLight>();
        for (auto [ent, light] : pointLightView.each())
        {
            std::string label = "PointLight " + std::to_string(i);

            if (ImGui::TreeNode(label.c_str()))
            {
                ImGui::DragFloat3(("Position##" + std::to_string(i)).c_str(), &light.Position[0],
                                  0.1f);
                ImGui::ColorEdit3(("Color##" + std::to_string(i)).c_str(), &light.Color[0]);
                ImGui::DragFloat(("Intensity##" + std::to_string(i)).c_str(), &light.Intensity,
                                 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat(("Range##" + std::to_string(i)).c_str(), &light.Range, 0.1f, 0.0f,
                                 100.0f);

                if (ImGui::Button(("Delete##" + std::to_string(i)).c_str()))
                {
                    toRemove.push_back(ent);
                }

                ImGui::TreePop();
            }

            i++;
        }
        for (auto ent : toRemove)
            nijiEngine.ecs.m_registry.destroy(ent);
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
    ImGui::End();
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
