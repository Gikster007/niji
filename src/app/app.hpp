#pragma once

#include "precomp.hpp"

class GLFWwindow;

class App
{
  public:
    void run();

  private:
    void init_window();
    void init_vulkan();

    void create_instance();

    void main_loop();

    void cleanup();

  public:

  private:
    GLFWwindow* m_window = nullptr;
    VkInstance m_instance;
};