#pragma once

#include <memory>

#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>

#include "rendering/model/model.hpp"

namespace niji
{

struct DirectionalLight
{
    DirectionalLight() = default;
    DirectionalLight(glm::vec3 dir, glm::vec3 color, float intensity)
        : Direction(dir), Color(color), Intensity(intensity)
    {
    }

    glm::vec3 Direction = {0.0f, -1.0f, 0.0f};
    glm::vec3 Color = {1.0f, 1.0f, 1.0f};
    float Intensity = 1.0f;
};

using json = nlohmann::json;
inline void to_json(json& j, const DirectionalLight& light)
{
    j = json{{"Direction", {light.Direction.x, light.Direction.y, light.Direction.z}},
             {"Color", {light.Color.x, light.Color.y, light.Color.z}},
             {"Intensity", light.Intensity}};
}

inline void from_json(const json& j, DirectionalLight& light)
{
    auto& dir = j.at("Direction");
    auto& col = j.at("Color");
    light.Direction = glm::vec3(dir[0], dir[1], dir[2]);
    light.Color = glm::vec3(col[0], col[1], col[2]);
    light.Intensity = j.at("Intensity");
}

// Each "Scene" Has Exactly 1 Directional Light, and it Stores Other Relevant Data (such as amount
// of point lights, etc.)
struct SceneInfo
{
    DirectionalLight DirLight = {};
    int PointLightCount = 0;
};

struct PointLight
{
    PointLight() = default;
    PointLight(glm::vec3 pos, glm::vec3 color, float intensity, float range)
        : Position(pos), Color(color), Intensity(intensity), Range(range)
    {
    }

    glm::vec3 Position = {0.0f, 0.0f, 0.0f};
    glm::vec3 Color = {1.0f, 0.0f, 0.0f};
    float Intensity = 1.0f;
    float Range = 1.0f;

    float _pad0 = -1.0f;
    float _pad1 = -1.0f;
};

inline void to_json(nlohmann::json& j, const PointLight& light)
{
    j = json{{"Position", {light.Position.x, light.Position.y, light.Position.z}},
             {"Color", {light.Color.x, light.Color.y, light.Color.z}},
             {"Intensity", light.Intensity},
             {"Range", light.Range}};
}
inline void from_json(const nlohmann::json& j, PointLight& light)
{
    auto& pos = j.at("Position");
    auto& col = j.at("Color");
    light.Position = glm::vec3(pos[0], pos[1], pos[2]);
    light.Color = glm::vec3(col[0], col[1], col[2]);
    light.Intensity = j.at("Intensity");
    light.Range = j.at("Range");
}

struct MeshComponent
{
    std::shared_ptr<Model> Model;
    uint16_t MeshID = -1;
    uint16_t MaterialID = -1;
};

struct Camera
{
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 3.0f);
    float Pitch = 0.0f;
    float Yaw = -90.0f;
    float Fov = 60.0f;
    float AspectRatio = 16.0f / 9.0f;
    float NearPlane = 0.1f;
    float FarPlane = 1000.0f;

    glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

    void UpdateVectors()
    {
        glm::vec3 front = {};
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
        Up = glm::normalize(glm::cross(Right, Front));
    }

    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    glm::mat4 GetProjectionMatrix() const
    {
        return glm::perspective(glm::radians(Fov), AspectRatio, NearPlane, FarPlane);
    }
};

} // namespace niji