#include "app.hpp"

#include <random>

#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

#include "../engine/core/components/render-components.hpp"
#include "../engine/core/components/transform.hpp"
#include "../engine/core/envmap.hpp"
#include "../engine/engine.hpp"

#include "camera_system.hpp"

using json = nlohmann::json;

static glm::vec3 get_rand_color()
{
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    return glm::vec3(dist(rng), dist(rng), dist(rng));
}

App::App()
{
    auto& renderer = nijiEngine.ecs.register_system<niji::Renderer>();

    auto entity = nijiEngine.ecs.create_entity();
    auto& t = nijiEngine.ecs.add_component<niji::Transform>(entity);
    // t.SetScale({1.0f, 1.0f, 1.0f});
    t.SetScale({0.01f, 0.01f, 0.01f});
    t.SetRotation(glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(0.0f), glm::radians(0.0f))));

    load_lights("assets/lights.json");

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
static int selectedPointLightIndex = -1;
void App::draw_light_editor()
{
    ImGui::Begin("Light Editor");

    ImGui::Text("Light Sets");
    for (int i = 0; i < m_lightSets.size(); ++i)
    {
        std::string label = "Set " + std::to_string(i) + ": " + m_lightSets[i].Name;
        if (ImGui::Selectable(label.c_str(), m_selectedLightSet == i))
            m_selectedLightSet = i;
    }

    if (ImGui::Button("Add New Light Set"))
    {
        LightSet newSet;
        newSet.Name = "Light Set " + std::to_string(m_lightSets.size());
        m_lightSets.push_back(newSet);
    }

    LightSet& set = m_lightSets[m_selectedLightSet];

    // Animation controls
    ImGui::Checkbox("Animate Lights", &set.Animate);
    ImGui::DragFloat2("Center", &set.Center[0], 0.1f, -10.0f, 10.0f);
    ImGui::DragFloat("X Radius", &set.RadiusX, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("Z Radius", &set.RadiusZ, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("Y Position", &set.YPos, 0.1f, -10.0f, 10.0f);
    ImGui::DragFloat("Speed", &set.Speed, 0.1f, -10.0f, 10.0f);

    ImGui::Checkbox("Toggle Debug Spheres", &set.DrawDebugSpheres);
    if (set.DrawDebugSpheres)
    {
        auto pointLightView = nijiEngine.ecs.m_registry.view<niji::PointLight>();
        for (const auto& ent : set.PointLights)
        {
            auto light = nijiEngine.ecs.try_get_component<niji::PointLight>(ent); 
            if (light)
                nijiEngine.add_sphere(light->Position, light->Range, light->Color, 64);
        }
    }

    // === Point Lights ===
    if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("Add Point Light"))
        {
            auto ent = nijiEngine.ecs.create_entity();
            nijiEngine.ecs.add_component<niji::PointLight>(ent, glm::vec3(0.0f), get_rand_color(),
                                                           10.0f, 1.0f);
            set.PointLights.push_back(ent);
        }

        int i = 0;
        std::vector<entt::entity> toRemove = {};
        for (auto ent : set.PointLights)
        {
            if (!nijiEngine.ecs.m_registry.valid(ent))
                continue;

            auto& light = nijiEngine.ecs.m_registry.get<niji::PointLight>(ent);
            std::string label = "PointLight " + std::to_string(i);
            bool selected = (selectedPointLightIndex == i);

            ImGui::PushID(i);

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow | (selected ? ImGuiTreeNodeFlags_Selected : 0);

            bool open = ImGui::TreeNodeEx(label.c_str(), flags);

            if (ImGui::IsItemClicked() && open)
                selectedPointLightIndex = selected ? -1 : i;

            if (open)
            {
                ImGui::DragFloat3("Position", &light.Position[0], 0.1f);
                ImGui::ColorEdit3("Color", &light.Color[0]);
                ImGui::DragFloat("Intensity", &light.Intensity, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("Range", &light.Range, 0.1f, 0.0f, 100.0f);

                if (ImGui::Button("Delete"))
                {
                    toRemove.push_back(ent);
                }

                ImGui::TreePop();
            }

            ImGui::PopID();

            // ImGuizmo translation
            if (selectedPointLightIndex == i)
            {
                auto& cameraSystem = nijiEngine.ecs.find_system<CameraSystem>();
                auto& camera = cameraSystem.m_camera;

                ImGuizmo::SetOrthographic(false);
                ImGuizmo::BeginFrame();

                ImDrawList* myDrawList = ImGui::GetForegroundDrawList();
                ImGuizmo::SetDrawlist(myDrawList);

                ImGuizmo::SetRect(ImGui::GetMainViewport()->WorkPos.x,
                                  ImGui::GetMainViewport()->WorkPos.y,
                                  ImGui::GetMainViewport()->WorkSize.x,
                                  ImGui::GetMainViewport()->WorkSize.y);

                glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), light.Position);
                glm::mat4 deltaMatrix = glm::mat4(1.0f);

                ImGuizmo::Manipulate(glm::value_ptr(camera.GetViewMatrix()),
                                     glm::value_ptr(camera.GetProjectionMatrix()),
                                     ImGuizmo::TRANSLATE, ImGuizmo::LOCAL,
                                     glm::value_ptr(modelMatrix), glm::value_ptr(deltaMatrix));

                if (ImGuizmo::IsUsing())
                {
                    glm::vec3 translation, rotation, scale;
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMatrix),
                                                          &translation.x, &rotation.x, &scale.x);
                    light.Position = translation;
                }
            }

            i++;
        }

        for (auto ent : toRemove)
        {
            // Remove from ECS
            nijiEngine.ecs.m_registry.destroy(ent);

            // Remove from this set
            auto it = std::find(set.PointLights.begin(), set.PointLights.end(), ent);
            if (it != set.PointLights.end())
                set.PointLights.erase(it);
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
    ImGui::End();
}

void App::rotate_point_lights()
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - startTime).count();

    for (auto& set : m_lightSets)
    {
        if (!set.Animate)
            continue;

        for (size_t i = 0; i < set.PointLights.size(); ++i)
        {
            auto ent = set.PointLights[i];
            if (!nijiEngine.ecs.m_registry.valid(ent))
                continue;

            auto& light = nijiEngine.ecs.m_registry.get<niji::PointLight>(ent);
            float angle = time * set.Speed + (i * glm::two_pi<float>() / set.PointLights.size());

            light.Position.x = set.Center.x + set.RadiusX * std::cos(angle);
            light.Position.z = set.Center.y + set.RadiusZ * std::sin(angle);
            light.Position.y = set.YPos;
        }
    }
}

void App::update(float deltaTime)
{
    draw_light_editor();

    rotate_point_lights();
}

void App::render()
{
}

void App::load_lights(std::string path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return;

    json root;
    file >> root;

    for (auto& jset : root["LightSets"])
    {
        LightSet set;
        set.Name = jset.value("Name", "Unnamed");
        auto center = jset.value("Center", std::vector<float>{0.f, 0.f});
        if (center.size() == 2)
            set.Center = glm::vec2(center[0], center[1]);

        set.RadiusX = jset.value("RadiusX", 8.0f);
        set.RadiusZ = jset.value("RadiusZ", 2.1f);
        set.YPos = jset.value("YPos", 3.4f);
        set.Speed = jset.value("Speed", 0.5f);
        set.Animate = jset.value("Animate", false);
        set.DrawDebugSpheres = jset.value("DrawDebugSpheres", false);

        for (auto& jlight : jset["PointLights"])
        {
            niji::PointLight light = jlight.get<niji::PointLight>();
            auto ent = nijiEngine.ecs.create_entity();
            nijiEngine.ecs.add_component<niji::PointLight>(ent, light);
            set.PointLights.push_back(ent);
        }

        m_lightSets.push_back(std::move(set));
    }

    for (auto& j : root["DirectionalLights"])
    {
        niji::DirectionalLight light = j.get<niji::DirectionalLight>();
        auto ent = nijiEngine.ecs.create_entity();
        nijiEngine.ecs.add_component<niji::DirectionalLight>(ent, light);
    }
}

void App::save_lights(std::string path)
{
    json root;

    for (const auto& set : m_lightSets)
    {
        json setJson;
        setJson["Name"] = set.Name;
        setJson["Animated"] = set.Animate;
        setJson["RadiusX"] = set.RadiusX;
        setJson["RadiusZ"] = set.RadiusZ;
        setJson["Center"] = {set.Center.x, set.Center.y};
        setJson["DrawDebugSpheres"] = set.DrawDebugSpheres;
        setJson["Speed"] = set.Speed;
        setJson["YPos"] = set.YPos;

        for (auto ent : set.PointLights)
        {
            if (!nijiEngine.ecs.m_registry.valid(ent))
                continue;
            if (!nijiEngine.ecs.m_registry.all_of<niji::PointLight>(ent))
                continue;

            const auto& light = nijiEngine.ecs.m_registry.get<niji::PointLight>(ent);
            setJson["PointLights"].push_back(
                light); // Assumes nlohmann::json has from_json for PointLight
        }

        root["LightSets"].push_back(setJson);
    }

    auto dirView = nijiEngine.ecs.m_registry.view<niji::DirectionalLight>();
    for (const auto& [ent, light] : dirView.each())
        root["DirectionalLights"].push_back(light);

    std::ofstream file(path);
    if (file.is_open())
        file << root.dump(4);
}

void App::cleanup()
{
    // Save Lights to File
    save_lights("assets/lights.json");

    for (auto& model : m_models)
    {
        // Hacky af but if it works, it works
        model->~Model();
    }
    m_envmap.cleanup();
}
