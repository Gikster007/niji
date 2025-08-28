#pragma once

#include "../engine/core/components/render-components.hpp"
#include "../engine/core/ecs.hpp"

class CameraSystem : public niji::System
{
  public:
    CameraSystem();
    ~CameraSystem();

    void update(float deltaTime) override;
    void render() override;

  private:
    void UpdateFlyCamera(GLFWwindow* window, float deltaTime);

  public:
    niji::Camera m_camera = {};
    bool m_isInsideViewport = false;
    bool m_checkViewportBounds = true;

  private:
    float m_lastX = 0.0f, m_lastY = 0.0f;
    bool m_firstMouse = true;
};