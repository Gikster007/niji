#include "app.hpp"

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
static int selectedPointLightIndex = -1;
static bool drawDebugSpheres = false;
static bool animateLights = true;
static float radius = 2.0f;
static float speed = 1.0f;
static float yPos = -0.6f;
static void DrawLightEditor()
{
    ImGui::Begin("Light Editor");

    ImGui::Checkbox("Toggle Debug Spheres", &drawDebugSpheres);
    if (drawDebugSpheres)
    {
        auto pointLightView = nijiEngine.ecs.m_registry.view<niji::PointLight>();
        for (auto [ent, light] : pointLightView.each())
        {
            nijiEngine.add_sphere(light.Position, light.Range, light.Color, 64);
        }
    }

    ImGui::Text("Animation Settings");
    ImGui::DragFloat("Radius", &radius);
    ImGui::DragFloat("Speed", &speed);
    ImGui::DragFloat("Y Position", &yPos);
    ImGui::Checkbox("Toggle Animation", &animateLights);

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
            bool selected = (selectedPointLightIndex == i);

            ImGui::PushID(static_cast<int>(i)); // Prevent ID collisions

            // Combined foldout + selectable
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                       /*ImGuiTreeNodeFlags_OpenOnDoubleClick |*/
                                       (selected ? ImGuiTreeNodeFlags_Selected : 0);

            bool open = ImGui::TreeNodeEx(label.c_str(), flags);

            // Handle selection (on single click)
            if (ImGui::IsItemClicked() && open)
            {
                selectedPointLightIndex = selected ? -1 : static_cast<int>(i);
            }

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

            // === ImGuizmo manipulation ===
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
                    // Extract translation from manipulated matrix
                    glm::vec3 translation, rotation, scale;
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMatrix),
                                                          &translation.x, &rotation.x, &scale.x);
                    light.Position = translation;
                }
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

static void RotatePointLights()
{
    if (!animateLights)
        return;

    glm::vec3 center = glm::vec3(0.0f); // Center of the circular motion

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration<float, std::chrono::seconds::period>(currentTime -
    startTime).count();

    int i = 0;
    auto pointLightView = nijiEngine.ecs.m_registry.view<niji::PointLight>();
    for (auto [ent, light] : pointLightView.each())
    {
        float angle =
            time * speed + (i * glm::two_pi<float>() / MAX_POINT_LIGHTS); // even spacing

        // Move the light in a circle on the XZ plane
        light.Position.x = center.x + radius * std::cos(angle);
        light.Position.z = center.z + radius * std::sin(angle);
        light.Position.y = yPos;
        
        i++;
    }
}

void App::update(float deltaTime)
{
    DrawLightEditor();

    RotatePointLights();
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
