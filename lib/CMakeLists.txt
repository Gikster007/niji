# GLM
# https://github.com/g-truc/glm
add_subdirectory("glm")
target_link_libraries(niji PRIVATE glm)
target_include_directories(niji PRIVATE "./glm/")

# GLFW
# https://github.com/glfw/glfw
#add_subdirectory("glfw")
#target_link_libraries(niji PRIVATE glfw)
target_include_directories(niji PRIVATE ${CMAKE_SOURCE_DIR}/lib/glfw/include)
set(GLFW_LIBRARY ${CMAKE_SOURCE_DIR}/lib/glfw/glfw3.lib)
target_link_libraries(niji PRIVATE ${GLFW_LIBRARY})

# STB_Image
# https://github.com/nothings/stb/blob/master/stb_image.h
target_include_directories(niji PRIVATE "./stb/")

# EnTT ECS
add_subdirectory("entt")
target_link_libraries(niji PUBLIC EnTT)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(niji PRIVATE Vulkan::Vulkan)