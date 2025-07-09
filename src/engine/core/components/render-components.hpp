#pragma once

#include <memory>

#include <glm/gtc/matrix_transform.hpp>

#include "rendering/model/model.hpp"

namespace niji
{

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