# GLM
# https://github.com/g-truc/glm
add_subdirectory("glm")
target_link_libraries(niji PRIVATE glm)
target_include_directories(niji PRIVATE "./glm/")

# GLFW
# https://github.com/glfw/glfw
add_subdirectory("glfw")
target_link_libraries(niji PRIVATE glfw)

# STB_Image
# https://github.com/nothings/stb/blob/master/stb_image.h
target_include_directories(niji PRIVATE "./stb/")

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(niji PRIVATE Vulkan::Vulkan)