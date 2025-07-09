#include "camera_system.hpp"

#include "../engine/engine.hpp"

CameraSystem::CameraSystem()
{
    m_camera = niji::Camera();
}

CameraSystem::~CameraSystem()
{
}

void CameraSystem::update(float deltaTime)
{
    UpdateFlyCamera(nijiEngine.m_context.m_window, deltaTime);
}

void CameraSystem::render()
{
}

void CameraSystem::UpdateFlyCamera(GLFWwindow* window, float deltaTime)
{
    bool enableMouseLook = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    if (enableMouseLook)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (m_firstMouse)
        {
            m_lastX = (float)xpos;
            m_lastY = (float)ypos;
            m_firstMouse = false;
        }

        float xoffset = (float)(xpos - m_lastX);
        float yoffset = (float)(m_lastY - ypos);
        m_lastX = (float)xpos;
        m_lastY = (float)ypos;

        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        m_camera.Yaw += xoffset;
        m_camera.Pitch += yoffset;

        m_camera.Pitch = glm::clamp(m_camera.Pitch, -89.0f, 89.0f);
        m_camera.UpdateVectors();
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_firstMouse = true;
    }

    // Movement (WASD + QE)
    float speed = 4.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= 3.0f;

    glm::vec3 moveDir(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        moveDir += m_camera.Front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        moveDir -= m_camera.Front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        moveDir -= m_camera.Right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        moveDir += m_camera.Right;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        moveDir += m_camera.Up;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        moveDir -= m_camera.Up;

    if (glm::length(moveDir) > 0.0f)
        m_camera.Position += glm::normalize(moveDir) * speed * deltaTime;
}